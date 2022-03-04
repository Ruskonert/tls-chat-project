import express, { Express, Request, Response } from 'express';

import cors     from 'cors';
import * as tls from 'tls';
import * as fs  from 'fs';
import * as body_parser from 'body-parser';

import {PackingMessage} from './client_type';
import { toEditorSettings } from 'typescript';

// 브로드캐스트 서버로부터 받은 메시지를 저장합니다.
const recvMessage: string[] = [];
var recvMessageSize = 0;


const options : tls.ConnectionOptions = {
  ca: [fs.readFileSync('cert/server.key')],
  requestCert: false,
  rejectUnauthorized: false
};


var broadcastStream : tls.TLSSocket;

/**
 * 메시지 리시버(브로드캐스트)와 연결합니다.
 */
const inputMessageStream : tls.TLSSocket = tls.connect(4433, options, () => {
  process.stdin.pipe(inputMessageStream);
  process.stdin.resume();
  var message : PackingMessage = PackingMessage.ConvertMessage(0, "CLIENT_VERSION 1.0.0");
  inputMessageStream.write(message.toString(), 'binary');

  /**
   * 메시지 리시버(브로드캐스트)와 연결합니다.
   */
  broadcastStream = tls.connect(8443, options, () => {
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
    recvMessageSize = recvMessage.length;
  
    var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
    broadcastStream.write(message.toString(), 'binary');
    broadcastStream.read();
  });

});


inputMessageStream.setEncoding('utf8');
inputMessageStream.on('data', (data : Buffer) => {
  console.log(`Response data: ${data}`);
});


const app : Express = express();
app.use(cors());
app.use(body_parser.urlencoded({ extended: false }));
app.use(body_parser.json());
const server = require('http').createServer(app);


app.get('/', (req : Request, res: Response) => {
  res.send({message: recvMessage});
});


app.post('/send', (req: Request, res: Response) => {
  const msg = PackingMessage.ConvertMessage(req.body.command, req.body.data);
  inputMessageStream.write(msg.toString(), 'binary');
  res.send({status: 'ok'});
});


server.listen(9999, () => {
  console.log('API Engine is running on 9999');
});