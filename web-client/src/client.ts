import express, { Express, Request, Response } from 'express';

import cors     from 'cors';
import * as tls from 'tls';
import * as fs  from 'fs';
import * as body_parser from 'body-parser';

import {PackingMessage} from './client_type';


// 브로드캐스트 서버로부터 받은 메시지를 저장합니다.
const recvMessage: string[] = [];
var recvMessageSize = 0;
var latestSize = 0;

const options : tls.ConnectionOptions = {
  ca: [fs.readFileSync('cert/server.key')],
  requestCert: false,
  rejectUnauthorized: false
};

var broadcastStream : tls.TLSSocket;

/**
 * 메시지 수신기(브로드캐스트)와 연결합니다.
 */
const inputMessageStream : tls.TLSSocket = tls.connect(4433, options, () => {
  process.stdin.pipe(inputMessageStream);
  process.stdin.resume();

  // 클라이언트 검증 절차를 수행합니다.
  var message : PackingMessage = PackingMessage.ConvertMessage(0, "CLIENT_VERSION 1.0.0");
  inputMessageStream.write(message.toString(), 'binary');


  /**
   * 클라이언트 검증 이후 메시지 수신기(브로드캐스트)와 연결합니다.
   */
  broadcastStream = tls.connect(8443, options, () => {
    process.stdin.pipe(broadcastStream);
    process.stdin.resume();

    // 메시지 수신을 위한 패킷 전송을 수행합니다.
    var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
    broadcastStream.write(message.toString(), 'binary');
  });

  broadcastStream.on('data', (data : Buffer) => {
    var str = Buffer.from(data).toString();
    console.log("Broadcast message received: " +  str.substring(12));
  
    recvMessage.push(str.substring(12));
    recvMessageSize = recvMessage.length;
  
    var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
    broadcastStream.write(message.toString(), 'binary');
  });
});


inputMessageStream.setEncoding('utf8');
inputMessageStream.on('data', (data : Buffer) => {
  console.log(`Clinet response data: ${data}`);
});


const app : Express = express();
app.use(cors());
app.use(body_parser.urlencoded({ extended: false }));
app.use(body_parser.json());
const server = require('http').createServer(app);




app.get('/', (req : Request, res: Response) => {
  while(true) {
    const len = recvMessage.length;
    if(latestSize < len) {
      res.send({message: recvMessage});
      latestSize = len;
      break;
    }
    else {
      continue;
    }
  }
});


app.post('/send', (req: Request, res: Response) => {
  const msg = PackingMessage.ConvertMessage(req.body.command, req.body.data);
  inputMessageStream.write(msg.toString(), 'binary');
  res.send({status: 'ok'});
});


server.listen(9999, () => {
  console.log('API Engine is running on 9999');
});