// code based on:
// https://gist.github.com/tedmiston/5935757

var net = require('net');

var client = new net.Socket();



function sleep(ms) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}



let received_data = "";

client.on("data", (data)=>{
	received_data += data.toString();

	if( received_data.includes('\n') ){
		const [first, ...rest] = received_data.split('\n');
		console.log(JSON.parse( first.toString()))
		client.end()
	}

});

client.connect(8001, '127.0.0.1', async function() {
	console.log('Connected');

	let request;
	request = {Cmd: "APP_BUILD"};
	console.log(request)
	client.write(JSON.stringify(request) + '\n')

});


