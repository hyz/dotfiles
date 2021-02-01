
window.URL.createObjectURL(blob));


<!DOCTYPE html>
<script id="worker1" type="javascript/worker">
  // This script won't be parsed by JS engines because its type is javascript/worker.
  self.onmessage = function(e) {
    self.postMessage('msg from worker');
  };
  // Rest of your worker code goes here.
</script>
<script>
  var blob = new Blob([
    document.querySelector('#worker1').textContent
  ], { type: "text/javascript" })

  // Note: window.webkitURL.createObjectURL() in Chrome 10+.
  var worker = new Worker(window.URL.createObjectURL(blob));
  worker.onmessage = function(e) {
    console.log("Received: " + e.data);
  }
  worker.postMessage("hello"); // Start the worker.
</script>


https://stackoverflow.com/questions/5408406/web-workers-without-a-separate-javascript-file


// Build a worker from an anonymous function body
var blobURL = URL.createObjectURL( new Blob([ '(',

function(){
    //Long-running work here
}.toString(),

')()' ], { type: 'application/javascript' } ) ),

worker = new Worker( blobURL );

// Won't be needing this anymore
URL.revokeObjectURL( blobURL );


