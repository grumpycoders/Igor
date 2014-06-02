/*
 * Construct an SRP object with a username,
 * password, and the bits identifying the 
 * group (1024 [default], 1536 or 2048 bits).
 */
SRPClient = function () {
  
  var initVal = this.initVals[1024];
  
  // Set N and g from initialization values.
  this.N = new BigInteger(initVal.N, 16);
  this.g = new BigInteger(initVal.g, 16);
  this.cNgHex = initVal.cNg;
  
  // Pre-compute k from N and g.
  this.k = this.k();
  
  // Convenience big integer objects for 1 and 2.
  this.zero = new BigInteger("0", 16);
  this.one = new BigInteger("1", 16);
  this.two = new BigInteger("2", 16);

  this.seq = this.one;
};

/*
 * Implementation of an SRP client conforming
 * to the SRP protocol 6A (see RFC5054).
 */
SRPClient.prototype = {

  clientSendPacketA: function(username, password) {

    this.I = username;
    this.p = password;
    this.a = this.srpRandom();
    this.A = this.calculateA(this.a);
	  
    return { clientPacketA: { A: this.A.toString(16), I: username }};

  },
        
  clientRecvPacketB: function(serverPacketB) {

    this.s = new BigInteger(serverPacketB.s, 16);
    this.B = new BigInteger(serverPacketB.B, 16);
   
    if (this.B.mod(this.N).equals(this.zero))
      return false;
	
    this.u = this.calculateU(this.A, this.B);
    this.S = this.calculateS(this.B, this.s, this.u, this.a);
    this.K = this.paddedHash([this.S.toString(16)]);
    return true;

  },
        
  clientSendProof: function() {

    this.M = this.calculateMc(this.cNgHex, this.s, this.A, this.B, this.K);
    return { clientProof: { M: this.M.toString(16) }};

  },

  clientRecvProof: function(serverProof) {

    return this.calculateMs(this.A, this.M, this.K).equals(new BigInteger(serverProof.M, 16));

  },

  generateProof: function() {

    var seqHex = this.seq.toString(16);
    var rndHex = this.srpRandom().toString(16);
    
    seq = seq.add(this.one);
    
    var subHashHex = this.paddedHash([seqHex, rndHex]).toString(16);
    var proofHex = this.paddedHash([subHashHex, this.K.toString(16)]);
    
    return { rnd: rndHex, seq: seqHex, prf: proofHex };
  
  },

  /*
   * Calculate k = H(N || g), which is used
   * throughout various SRP calculations.
   */
  k: function() {
    
    // Convert to hex values.
    var toHash = [
      this.N.toString(16),
      this.g.toString(16)
    ];
    
    // Return hash as a BigInteger.
    return this.paddedHash(toHash);

  },
  
  /*
   * Calculate x = SHA1(s | SHA1(I | ":" | P))
   */
  calculateX: function (salt) {
    
    // Verify presence of parameters.
    if (!salt) throw 'Missing parameter.'
    
    if (!this.I || !this.p)
      throw 'Username and password cannot be empty.';
    
    // Hash the concatenated username and password.
    var usernamePassword = this.I + ":" + this.p;
    var usernamePasswordHash = this.hash(usernamePassword);
    
    // Calculate the hash of salt + hash(username:password).
    return this.paddedHash([salt.toString(16), usernamePasswordHash]);
  },
  
  /*
   * Calculate u = SHA1(PAD(A) | PAD(B)), which serves
   * to prevent an attacker who learns a user's verifier
   * from being able to authenticate as that user.
   */
  calculateU: function(A, B) {
    
    // Verify presence of parameters.
    if (!A || !B) throw 'Missing parameter(s).';
    
    // Verify value of A and B.
    if (A.mod(this.N).equals(this.zero) ||
        B.mod(this.N).equals(this.zero))
      throw 'ABORT: illegal_parameter';
    
    // Convert A and B to hexadecimal.
    var toHash = [A.toString(16), B.toString(16)];
    
    // Return hash as a BigInteger.
    return this.paddedHash(toHash);

  },
  
  /*
   * 2.5.4 Calculate the client's public value A = g^a % N,
   * where a is a random number at least 256 bits in length.
   */
  calculateA: function(a) {
    
    // Verify presence of parameter.
    if (!a) throw 'Missing parameter.';
    
    // Return A as a BigInteger.
    var A = this.g.modPow(a, this.N);
    
    if (A.mod(this.N).equals(this.zero))
      throw 'ABORT: illegal_parameter';
    
    return A;
    
  },
  
  /*
   * Calculate match M = H(A, M, K)
   */
  calculateMs: function (A, M, K) {
    
    // Verify presence of parameters.
    if (!A || !M || !K)
      throw 'Missing parameter(s).';
    
    // Verify value of A and B.
    if (A.mod(this.N).equals(this.zero) ||
        M.mod(this.N).equals(this.zero))
      throw 'ABORT: illegal_parameter';
    
    var aHex = A.toString(16);
    var bHex = M.toString(16);
    var kHex = K.toString(16);
    
    var array = [aHex, bHex, kHex];

    return this.paddedHash(array);
    
  },
  
  /*
   * Calculate match M = H(H(N) ^ H(g), H(I), s, A, B, K)
   */
  calculateMc: function (cp, salt, A, B, K) {

    // Verify presence of parameters.
    if (!cp || !salt || !A || !B || !K)
      throw 'Missing parameter(s).';
    
    // Verify value of A and B.
    if (A.mod(this.N).equals(this.zero) ||
        B.mod(this.N).equals(this.zero))
      throw 'ABORT: illegal_parameter';
    
    var hl = this.hash(this.I).toString(16);
    
    return this.paddedHash([cp, hl, salt.toString(16), A.toString(16), B.toString(16), K.toString(16)]);

  },
  
  /*
   * Calculate the client's premaster secret 
   * S = (B - (k * g^x)) ^ (a + (u * x)) % N
   */
  calculateS: function(B, salt, uu, aa) {
    
    // Verify presence of parameters.
    if (!B || !salt || !uu || !aa)
      throw 'Missing parameters.';
    
    // Verify value of B.
    if (B.mod(this.N).equals(this.zero))
      throw 'ABORT: illegal_parameter';
      
    // Calculate X from the salt.
    var x = this.calculateX(salt);
    var str = x.toString(16);
    
    // Calculate bx = g^x % N
    var bx = this.g.modPow(x, this.N);
    
    // Calculate ((B + N * k) - k * bx) % N
    var btmp = B.add(this.N.multiply(this.k))
    .subtract(bx.multiply(this.k)).mod(this.N);
    
    // Finish calculation of the premaster secret.
    return btmp.modPow(x.multiply(uu).add(aa), this.N);
  
  },
  
  calculateK: function (S) {

    return this.hexHash(S.toString(16));

  },
  
  /*
   * Helper functions for random number
   * generation and format conversion.
   */
  
  /* Generate a random big integer */
  srpRandom: function() {

    var words = sjcl.random.randomWords(8,0);
    var hex = sjcl.codec.hex.fromBits(words);
    
    // Verify random number large enough.
    if (hex.length != 64)
      throw 'Invalid random number size.'

    var r = new BigInteger(hex, 16);
    
    if (r.compareTo(this.N) >= 0)
      r = a.mod(this.N.subtract(this.one));

    if (r.compareTo(this.two) < 0)
      r = two;

    return r;

  },
  
  /*
   * Helper functions for hasing/padding.
   */

  /*
  * SHA1 hashing function with padding: input 
  * is prefixed with 0 to meet N hex width.
  */
  paddedHash: function (array) {

   var nlen = 2 * ((this.N.toString(16).length * 4 + 7) >> 3);

   var toHash = '';
   
   for (var i = 0; i < array.length; i++) {
     toHash += this.nZeros(nlen - array[i].length) + array[i];
   }
   
   var hash = new BigInteger(this.hexHash(toHash), 16);
   
   return hash.mod(this.N);

  },

  /* 
   * Generic hashing function.
   */
  hash: function (str) {
    return calcSHA1(str);

  },
  
  /*
   * Hexadecimal hashing function.
   */
  hexHash: function (str) {
    return this.hash(this.pack(str));

  },
  
  /*
   * Hex to string conversion.
   */
  pack: function(hex) {
    
    // To prevent null byte termination bug
    if (hex.length % 2 != 0) hex = '0' + hex;
    
    i = 0; ascii = '';

    while (i < hex.length/2) {
      ascii = ascii+String.fromCharCode(
      parseInt(hex.substr(i*2,2),16));
      i++;
    }

    return ascii;

  },
  
  /* Return a string with N zeros. */
  nZeros: function(n) {
    
    if(n < 1) return '';
    var t = this.nZeros(n >> 1);
    
    return ((n & 1) == 0) ?
      t + t : t + t + '0';
  
  },
  
  /*
   * SRP group parameters, composed of N (hexadecimal
   * prime value) and g (decimal group generator).
   * See http://tools.ietf.org/html/rfc5054#appendix-A
   */
  initVals: {    
    1024: {
      N: 'EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C' +
         '9C256576D674DF7496EA81D3383B4813D692C6E0E0D5D8E250B98BE4' +
         '8E495C1D6089DAD15DC7D7B46154D6B6CE8EF4AD69B15D4982559B29' +
         '7BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA9A' +
         'FD5138FE8376435B9FC61D2FC0EB06E3',
      g: '2',
      cNg: '95CBBA26F8E9B1415C71417A4D63D1F7D7CDED7B',
    },
  },
};
