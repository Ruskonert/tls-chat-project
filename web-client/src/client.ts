import express, { Express, Request, Response } from 'express';
import cors     from 'cors';
import * as tls from 'tls';
import * as fs  from 'fs';

// Packing Message를 정의합니다.
class PackingMessage
{
  len : number;
  
  command: number;
  
  data: string;

  
  static ConvertMessage(command : number, data: string)
  {
    var newMessage = new PackingMessage(data.length, command, data);
    return newMessage;
  }


  private constructor(len: number, command_type: number, data: string)
  {
    this.len = len;
    this.command = command_type;
    this.data = data;
  }


  public toString() : string
  {
    var dataLengthStr = this.data.length.toString(16).padStart(4, '0');
    var commandStr = this.command.toString(16).padStart(4, '0');
    var magicNumber = '';

    magicNumber += String.fromCharCode(0x99);
    magicNumber += String.fromCharCode(0x88);
    magicNumber += String.fromCharCode(0x77);
    magicNumber += String.fromCharCode(0x66);

    var completeStr = `${magicNumber}${commandStr}${dataLengthStr}${this.data}`;
    return completeStr;
  }
}

// 브로드캐스트 서버로부터 받은 메시지를 저장합니다.
const recvMessage: string[] = [];

const options : tls.ConnectionOptions = {
  ca: [fs.readFileSync('cert/server.key')],
  requestCert: false,
  rejectUnauthorized: false
};


/**
 * 메시지 리시버(브로드캐스트)와 연결합니다.
 */
const inputMessageStream : tls.TLSSocket = tls.connect(4433, options, () => {
  process.stdin.pipe(inputMessageStream);
  process.stdin.resume();
  var message : PackingMessage = PackingMessage.ConvertMessage(0, "CLIENT_VERSION 1.0.0");
  inputMessageStream.write(message.toString(), 'binary');
});

inputMessageStream.setEncoding('utf8');

inputMessageStream.on('data', (data : Buffer) => {
  console.log(`Response data: ${data}`);
});


/**
 * 메시지 리시버(브로드캐스트)와 연결합니다.
 */
const broadcastStream : tls.TLSSocket = tls.connect(8443, options, () => {
  process.stdin.pipe(broadcastStream);
  process.stdin.resume();

  var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
  broadcastStream.write(message.toString(), 'binary');
  broadcastStream.read();
});


broadcastStream.on('data', (data : Buffer) => {
  var str = Buffer.from(data).toString();
  console.log("Message Received: " +  str.substring(12));

  recvMessage.push(str.substring(12));

  var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
  broadcastStream.write(message.toString(), 'binary');
  broadcastStream.read();
});


const app : Express = express();
const server = require('http').createServer(app);

app.use(cors());

app.get('/', (req : Request, res: Response) => {
  res.send({message: recvMessage});
});

app.post('/send', (req: Request, res: Response) => {

});

server.listen(9999, () => {
  console.log('API Engine is running on 9999');
});