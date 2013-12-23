<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'
'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>
<html xmlns='http://www.w3.org/1999/xhtml'>
  <head>
    <meta http-equiv='content-type' content='text/html; charset=utf-8' />
    <title>Igor</title>
    <link rel='stylesheet' href='{{dojo_path}}/dijit/themes/claro/claro.css'>

    <style type='text/css'>
      html, body {
        height: 100%; width: 100%;
        padding: 0; border: 0; margin: 0;
        background: #fff url("/static/img/bgBody.gif") repeat-x top left;
      }
      .claro {
        font: 12px Myriad,Helvetica,Tahoma,Arial,clean,sans-serif; 
        *font-size: 75%;
      }
      /* pre-loader specific stuff to prevent unsightly flash of unstyled content */
      #loader {
        padding: 0; border: 0; margin: 0;
        position: absolute;
        top:0; left:0;
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
        width: 175px;
        background:#33c;
        color:#fff;
      }
      
      #main {
        height: 100%; width: 100%;
//        padding: 0; border: 0; margin: 0;
      }
      
//      #navMenu {
//        padding: 0; border: 0; margin: 0;
//      }
      hr.spacer { border:0; background-color: #ededed; width: 80%; height: 1px; }
    </style>
    <script src='{{dojo_path}}/dojo/dojo.js' data-dojo-config='has:{"dojo-firebug": true}, async: true'></script>
    <script>
      var reloadUIAction;

      require([
        'dojo', 
        'dijit/dijit', 
        'dojo/parser', 
        'dojo/ready', 
        'dojo/dom', 
        'dojo/on', 
        'dojo/request', 
        'dojo/json',
        'dijit/dijit-all', 
        'dojox/socket', 
        'dojox/socket/Reconnect', 
        'dojo/domReady!'], 
      function(dojo, dijit, parser, ready, dom, on, request, json) {
        var errorDlg;

        ready(function() {
          dojo.fadeOut({
            node: 'loader',
            duration: 500,
            onEnd: function(n) { n.style.display = 'none'; }
          }).play();
          parser.parse();
          
          errorDlg = new dijit.Dialog({
            title: "Error",
            style: "width: 300px"
          });
        });
        var socket = dojox.socket({url:'/dyn/igorws'});
        socket = dojox.socket.Reconnect(socket);

        dojo.connect(socket, 'onopen', function(event) {
        });

        dojo.connect(socket, 'onmessage', function(event) {
          var message = event.data;
        });
        
        var showError = function(str) {
          errorDlg.set("content", str);
          errorDlg.show();
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
      });

    </script>
  </head>

  <body class='claro'>
    <div id='loader'><div id='loaderInner' style='direction:ltr;'>Loading...</div></div>
    <div id='main' data-dojo-type='dijit/layout/BorderContainer' data-dojo-props='liveSplitters:false, design:"sidebar"'>
      <div id='navMenu' data-dojo-type='dijit/MenuBar' data-dojo-props='region:"top"'>
        <div data-dojo-type='dijit/PopupMenuBarItem'>
          <span>File</span>
          <div data-dojo-type='dijit/Menu'>
            <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick: reloadUIAction'>Reload UI</div>
          </div>
        </div>
        <div data-dojo-type='dijit/PopupMenuBarItem'>
          <span>Help</span>
          <div data-dojo-type='dijit/Menu'>
            <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick: function() { }'>About</div>
          </div>
        </div>
      </div>
      
      <div data-dojo-type='dijit/layout/AccordionContainer' data-dojo-props='region:"leading", splitter:true, minSize:200' style='width:300px;'>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Symbols"'>
        </div>
      </div>
      
      <div data-dojo-type='dijit/layout/TabContainer' data-dojo-props='region:"center"', tabStrip:'true', id='topTabs'>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Disassembly", style:"padding:10px;display:none;"'>
        </div>
        <div data-dojo-type='dijit/layout/ContentPane' data-dojo-props='title:"Memory View", style:"padding:10px;display:none;"'>
        </div>
      </div>
    </div>
  </body>
</html>
