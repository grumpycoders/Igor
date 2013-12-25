<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'
'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>
<html xmlns='http://www.w3.org/1999/xhtml'>
  <head>
    <meta http-equiv='content-type' content='text/html; charset=utf-8' />
    <title>Igor</title>
    <link rel='stylesheet' href='{{dojo_path}}/dojo/resources/dojo.css' />
    <link rel='stylesheet' href='{{dojo_path}}/dijit/themes/claro/claro.css' />
    <link rel='stylesheet' href='{{dojo_path}}/dojox/grid/resources/Grid.css' />
    <link rel='stylesheet' href='{{dojo_path}}/dojox/grid/resources/claroGrid.css' />
    <link rel='stylesheet' href='/static/js/dgrid/css/dgrid.css' />
    <link rel='stylesheet' href='/static/js/dgrid/css/skins/claro.css' />

    <style type='text/css'>
      html, body {
        height: 100%; width: 100%;
        padding: 0; border: 0; margin: 0;
        background: #CDDDE9 url("/static/img/bgBody.gif") repeat-x top left;
      }
      .claro {
        font: 12px Myriad, Helvetica, Tahoma, Arial, clean, sans-serif; 
        *font-size: 75%;
      }
      
      /* pre-loader specific stuff to prevent unsightly flash of unstyled content */
      #loader {
        padding: 0; border: 0; margin: 0;
        position: absolute;
        top: 0; left:0;
        width: 100%; height: 100%;
        background: #ededed;
        z-index: 999;
        vertical-align: middle;
      }

      #loaderInner {
        padding: 5px;
        position: relative;
        left: 0;
        top: 0;
        width: 370px;
        height: 50px;
        color: #fff;
        border: 1px solid;
        border-color: #000;
        margin: auto;
        vertical-align: middle;
        
        font: 30px sans-serif;
        
        text-align: center;
        
        background: #7d7e7d; /* Old browsers */
        background: -moz-linear-gradient(top,  #7d7e7d 0%, #0e0e0e 100%); /* FF3.6+ */
        background: -webkit-gradient(linear, left top, left bottom, color-stop(0%,#7d7e7d), color-stop(100%,#0e0e0e)); /* Chrome,Safari4+ */
        background: -webkit-linear-gradient(top,  #7d7e7d 0%,#0e0e0e 100%); /* Chrome10+,Safari5.1+ */
        background: -o-linear-gradient(top,  #7d7e7d 0%,#0e0e0e 100%); /* Opera 11.10+ */
        background: -ms-linear-gradient(top,  #7d7e7d 0%,#0e0e0e 100%); /* IE10+ */
        background: linear-gradient(to bottom,  #7d7e7d 0%,#0e0e0e 100%); /* W3C */
        filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='#7d7e7d', endColorstr='#0e0e0e',GradientType=0 ); /* IE6-9 */
      }
      
      .loaderVPad {
        height: 30%;
      }
      
      /* pre-loader end */
      
      #main {
        height: 100%; width: 100%;
/*        padding: 0; border: 0; margin: 0; */
      }
      
/*    #navMenu {
        padding: 0; border: 0; margin: 0;
      }
*/

      #hexView {
        height: 100%;
      }
      
      #statusBar {
        padding: 0; margin: 0;
        background-color: #efefef;
        background-repeat: repeat-x;
        background-image: -moz-linear-gradient(rgba(255, 255, 255, 0.7) 0%, rgba(255, 255, 255, 0) 100%);
        background-image: -webkit-linear-gradient(rgba(255, 255, 255, 0.7) 0%, rgba(255, 255, 255, 0) 100%);
        background-image: -o-linear-gradient(rgba(255, 255, 255, 0.7) 0%, rgba(255, 255, 255, 0) 100%);
        background-image: linear-gradient(rgba(255, 255, 255, 0.7) 0%, rgba(255, 255, 255, 0) 100%);
        _background-image: none;
      }
      
      #statusBarLayoutContainer {
        width: 100%;
        height: 32px; /* humf */
      }
      
      #clock {
        border-style: solid;
        border-color: #b5bcc7;
        border-top-width: 0px;
        border-bottom-width: 0px;
        border-right-width: 0px;
        border-left-width: 1px;
        width: 35px;
        text-align: center;
      }
      
      .hexViewContent {
        font-family: monospace;
        text-align: center;
        border: 1px transparent solid;
        border-color: transparent transparent #E5DAC8 transparent;
        padding: 1px 1px 2px 3px;
      }
      
      .hexViewAddress {
        font-family: monospace;
        text-align: right;
        border: 1px solid transparent;
        border-color: transparent #E5DAC8 transparent transparent;
        padding: 1px 5px 2px 2px;
      }
      
      #hexView .dojoxGrid .dojoxGridCell {
        border-width: 0px;
        padding: 0px;
      }
      
      hr.spacer { border:0; background-color: #ededed; width: 80%; height: 1px; }
    </style>
    <script>
      dojoConfig = {
        has: { 'dojo-firebug': true },
        async: true,
        packages: [
        { name: 'dgrid', location: location.pathname.replace(/\/[^/]+$/, '') + '/../static/js/dgrid' },
        { name: 'xstyle', location: location.pathname.replace(/\/[^/]+$/, '') + '/../static/js/xstyle' },
        { name: 'put-selector', location: location.pathname.replace(/\/[^/]+$/, '') + '/../static/js/put-selector' },
        ]
      };
    </script>
    <script src='{{dojo_path}}/dojo/dojo.js'></script>
    <script>
      var reloadUIAction;
      var buildHexView;
      var buildDgridHexView;
      var showError;
      var showNotification;
      var socket;
      var messageListeners = { }
      var sendMessage;

      String.prototype.repeat = function(num) {
        return new Array(num + 1).join(this);
      }

      var hexPadding = function(val, s) {
        val = val.toString(16).toUpperCase();
        var n = val.length;
        s = s * 2;
        if (n >= s)
          return val;
        return '0'.repeat(s - n) + val;
      }
      
      var registerMessageListener = function(message, listener) {
        messageListeners[message] = listener;
      }

      require([
        'dojo',
        'dijit/dijit',
        'dojo/_base/lang',
        'dojo/parser',
        'dojo/dom',
        'dojo/on',
        'dojo/request',
        'dojo/json',
        'dojox/grid/DataGrid',
        'dojo/data/ItemFileWriteStore',
        'dgrid/Grid',
        'dojo/dom-form',
        'dijit/dijit-all',
        'dojox/socket',
        'dojox/socket/Reconnect',
        'dojo/domReady!'],
      function(dojo, dijit, lang, parser, dom, on, request, json, DataGrid, ItemFileWriteStore, Grid, domForm) {
        var errorDlg;
        var clock;

        var formatterAddress = function(addr) {
          addr = json.parse(addr);
          var value = hexPadding(addr.value, 4);
          return '<div class="hexViewAddress">' + value + '</div>';
        }

        var formatterContent = function(content) {
          content = json.parse(content);
          var value = hexPadding(content.value, 1);
          return '<div class="hexViewContent">' + value + '</div>';
        }

        showError = function(str) {
          errorDlg.set("content", str);
          errorDlg.show();
        }
        
        showNotification = function(str) {
          notifyDlg.set("content", str);
          notifyDlg.show();
        }

        reloadUIAction = function() {
          request.get('/dyn/reloadui').then(
            function(retStr) {
              var ret = json.parse(retStr);
              if (ret.success) {
                location.reload(true);
              } else {
                showError("Server couldn't load template: " + ret.msg);
              }
            },
            function(error) {
              showError("Error while loading reloadui: " + error);
            }
          );
        };
        parser.parse();
        
        clock = dojo.byId('clock');

        dojo.fadeOut({
          node: 'loader',
          duration: 500,
          onEnd: function(n) { n.style.display = 'none'; }
        }).play();

        errorDlg = new dijit.Dialog({
          title: "Error",
          style: "width: 300px"
        });
        
        notifyDlg = new dijit.Dialog({
          title: "Notification",
          style: "width: 300px"
        });

        buildHexView = function(nCols) {
          var data = {
            identifier: 'address',
            items: []
          }

          for (var r = 0; r < 20; r++) {
            var row = { address: '{ value: ' + (r * nCols) + ' }' }
            for (var c = 0; c < nCols; c++) {
              var cName = 'c' + c;
              var cell = { };
              cell[cName] = '{ "value": 0 }';
              lang.mixin(row, cell);
            }
            data.items.push(row);
          }

          var store = new ItemFileWriteStore({ data: data });

          var layout = [[ { name: ' ', field: 'address', width: '80px', formatter: formatterAddress } ]];

          for (var c = 0; c < nCols; c++) {
            var cName = 'c' + c;
            var col = { width: '20px', formatter: formatterContent, field: cName, name: ' ' };
            layout[0][c + 1] = col;
          }

          var grid = new DataGrid({
            id: 'hexViewGrid',
            store: store,
            structure: layout,
            rowSelector: '0px'
          });

          dojo.byId("hexView").appendChild(grid.domNode);
          grid.startup();
        }
        
        buildDgridHexView = function(nCols) {
          var columns = [{ field: 'address', label: ' '}];
          
          for (var c = 0; c < nCols; c++) {
            var cname = 'c' + c;
            columns[c + 1] = { label: ' ', field: cname };
          }
          
          var data = [];
          
          for (var r = 0; r < 20; r++) {
            var row = { address: r * nCols };
            for (var c = 0; c < nCols; c++) {
              var cname = 'c' + c;
              row[cname] = 0;
            }
            data.push(row);
          }
          
          var grid = new Grid({ columns: columns }, "dgridHexView");
          grid.renderArray(data);
        }

        buildHexView(16);
        buildDgridHexView(16);

        socket = dojox.socket({url:'/dyn/igorws'});
        socket = dojox.socket.Reconnect(socket);

        dojo.connect(socket, 'onopen', function(event) {
        });

        dojo.connect(socket, 'onmessage', function(event) {
          var message = json.parse(event.data);
          messageListeners[message.destination](message.call, message.data);
        });

        registerMessageListener('status', function(call, msg) {
          if (call == 'clock') {
            clock.innerHTML = msg.clock;
          }
        });
        
        registerMessageListener('main', function(call, msg) {
          if (call == 'notify') {
            showNotification(msg.message);
          }
        });
        
        sendMessage = function(destination, call, data) {
          socket.send(json.stringify({ destination: destination, call: call, data: data }));
        }
      });
    </script>
  </head>

  <body class='claro'>
    <div id='loader'>
      <div class='loaderVPad'></div>
      <div id='loaderInner'>Loading...</div>
    </div>
    <div id='main' data-dojo-type='dijit/layout/BorderContainer' data-dojo-props='liveSplitters:false, design:"sidebar"'>
      <div id='navMenu' data-dojo-type='dijit/MenuBar' data-dojo-props='region:"top"'>
        <div data-dojo-type='dijit/PopupMenuBarItem'>
          <span>File</span>
          <div data-dojo-type='dijit/Menu'>
            <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick: reloadUIAction'>Reload UI</div>
          </div>
        </div>
        <div data-dojo-type='dijit/PopupMenuBarItem'>
          <span>Network</span>
          <div data-dojo-type='dijit/Menu'>
            <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick: function() { broadcastEmitForm.reset(); broadcastEmitDialog.show(); }'>Broadcast</div>
          </div>
        </div>
        <div data-dojo-type='dijit/PopupMenuBarItem'>
          <span>Help</span>
          <div data-dojo-type='dijit/Menu'>
            <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick: function() { }'>About</div>
          </div>
        </div>
      </div>
      
      <div data-dojo-type='dijit/layout/AccordionContainer' data-dojo-props='region:"left", splitter:true, minSize:200' style='width:300px;'>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Symbols"'>
        </div>
      </div>
      
      <div data-dojo-type='dijit/layout/TabContainer' data-dojo-props='region:"center"', tabStrip:'true', id='topTabs'>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Disassembly", style:"padding:10px;display:none;"'>
        </div>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Memory View", style:"padding:10px;display:none;"'>
          <div id='hexView'></div>
        </div>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Memory View (dgrid)", style:"padding:10px;display:none;"'>
          <div id='dgridHexView'></div>
        </div>
      </div>
      
      <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='region:"bottom"' id='statusBar'>
        <div data-dojo-type='dijit/layout/LayoutContainer' id='statusBarLayoutContainer'>
          <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='region:"center"' id='statusMain'>Igor</div>
          <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='region:"right", layoutPriority:1' id='clock'>xx:xx</div>
        </div>
      </div>
    </div>
    
    <div data-dojo-type='dijit/Dialog' data-dojo-id='broadcastEmitDialog' title='Broadcast message' style='display: none'>
      <form data-dojo-type='dijit/form/Form' data-dojo-id='broadcastEmitForm'>
        <script type='dojo/on' data-dojo-event='submit' data-dojo-args='e'>
          e.preventDefault();
          if (!broadcastEmitForm.isValid()) { return; }
          sendMessage('main', 'broadcast', broadcastEmitForm.value);
          broadcastEmitDialog.hide();
        </script>
        <div class='dijitDialogPaneContentArea'>
          <label for='message'>Message: </label>
          <input type='text' name='message' id='message' required='true' data-dojo-type='dijit/form/ValidationTextBox' />
        </div>
        <div class='dijitDialogPaneActionBar'>
          <button data-dojo-type='dijit/form/Button' type='submit'>Ok</button>
          <button data-dojo-type='dijit/form/Button' type='button' data-dojo-props='onClick:function() { broadcastEmitDialog.hide(); }'>Cancel</button>
        </div>
      </form>
    </div>
  </body>
</html>
