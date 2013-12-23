<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'
'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>
<html xmlns='http://www.w3.org/1999/xhtml'>
  <head>
    <meta http-equiv='content-type' content='text/html; charset=utf-8' />
    <title>Igor</title>
    <link rel='stylesheet' href='{{dojo_path}}/dijit/themes/claro/claro.css'>

    <style type='text/css'>
      html, body { height: 100%; width: 100%; padding: 0; border: 0; }
      .claro {
        font-family: Verdana, Arial, Helvetica, sans-serif;
      }
      #main { height: 100%; width: 100%; border: 0; }
      #header { margin: 0; }
      /* pre-loader specific stuff to prevent unsightly flash of unstyled content */
      #loader {
        padding: 0;
        margin: 0;
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
      hr.spacer{ border:0; background-color: #ededed; width: 80%; height: 1px; }
    </style>
    <script src='{{dojo_path}}/dojo/dojo.js' data-dojo-config='has:{"dojo-firebug": true}, async: true'></script>
    <script>
      require([
        'dojo', 
        'dijit/dijit', 
        'dojo/parser', 
        'dojo/ready', 
        'dojo/dom', 
        'dojo/on', 
        'dojo/request', 
        'dijit/dijit-all', 
        'dojox/socket', 
        'dojox/socket/Reconnect', 
        'dojo/domReady!'], 
      function(dojo, dijit, parser, ready, dom, on, request) {
        ready(function() {
          dojo.fadeOut({
            node: 'loader',
            duration: 500,
            onEnd: function(n) { n.style.display = 'none'; }
          }).play();
          parser.parse();
        });
        var socket = dojox.socket({url:'/dyn/igorws'});
        socket = dojox.socket.Reconnect(socket);

        dojo.connect(socket, 'onopen', function(event) {
        });

        dojo.connect(socket, 'onmessage', function(event) {
          var message = event.data;
        });

      });
    </script>
  </head>

  <body class='claro'>
    <div id='loader'><div id='loaderInner' style='direction:ltr;'>Loading...</div></div>
    <div data-dojo-type='dijit/MenuBar', id='navMenu'>
      <div data-dojo-type='dijit/PopupMenuBarItem'>
        <span>File</span>
        <div data-dojo-type='dijit/DropDownMenu' id='fileMenu'>
          <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick:function() { alert("open"); }'>Open</div>
        </div>
      </div>
    </div>
  </body>
</html>
