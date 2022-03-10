import express, { Express, Request, Response } from 'express';

import cors             from 'cors';
import * as tls         from 'tls';
import * as fs          from 'fs';
import * as body_parser from 'body-parser';
import {PackingMessage, isPackingMessage } from './client_type';


const options : tls.ConnectionOptions = {
  ca: [fs.readFileSync('cert/server.key')],
  requestCert: false,
  rejectUnauthorized: false
};


const recvMessage : string[] = [];

var inputMessageStream : tls.TLSSocket;
var broadcastStream    : tls.TLSSocket;
var broadcastStream    : tls.TLSSocket;
var isVerifed          : boolean = false;
var changed            : boolean = false;


// 현재 시간을 반환합니다.
function getCurrentTimeToString() : string
{
  let date_ob = new Date();

  // current date
  // adjust 0 before single digit date
  let date = ("0" + date_ob.getDate()).slice(-2);
  
  // current month
  let month = ("0" + (date_ob.getMonth() + 1)).slice(-2);
  
  // current year
  let year = date_ob.getFullYear();
  
  // current hours
  let hours = date_ob.getHours();
  
  // current minutes
  let minutes = date_ob.getMinutes();
  
  // current seconds
  let seconds = date_ob.getSeconds();

  return year + "-" + month + "-" + date + " " + hours + ":" + minutes + ":" + seconds;
}


function insertNewMessage(str: string) {
  recvMessage.push(`[${getCurrentTimeToString()}] ${str}`);
  changed = true;
}


// 클라이언트 검증 절차를 수행합니다.
function verifyClientHandshake (socket : tls.TLSSocket) {
  var message : PackingMessage = PackingMessage.ConvertMessage(0, "CLIENT_VERSION 1.0.0");
  socket.write(message.toString(), 'binary');
}


// 수신받은 바이너리 데이터를 해석합니다.
function analysisRecvBinaryData(response : any) {
  if(isPackingMessage(response)) {
    var p = PackingMessage.ConvertMessageFromBinary(response);
    if(p == null) { 
      console.log("Invaild packing message.");
    }
    if(p?.command == 0x01) {
      insertNewMessage(`${p.data}\n`);
    }
  }
  else {
    switch(response) {
      case "WELCOME":
        insertNewMessage("클라이언트 검증에 성공하였습니다. 브로드캐스트 서버에 연결합니다...\n");
        isVerifed = true;
        startBroadcastClient();
        break;
      case "OK":
        //insertNewMessage("명령을 잘 수행하였습니다.\n");
        break;
      case "FAIL":
        insertNewMessage("명령을 수행하지 못했습니다.\n");
        break;
      case "NOSUPPORT":
        insertNewMessage("서버에서 지원하지 않는 명령입니다.\n");
        break;
      case "BYE":
        insertNewMessage("서버에서 연결을 끊었습니다.\n");
        inputMessageStream.destroy();
        break;
    }
  }
}


// 메시지 전송기 서버와 연결합니다.
inputMessageStream  = tls.connect(4433, options, () => {
  process.stdin.pipe(inputMessageStream);
  process.stdin.resume();

  inputMessageStream.setEncoding('utf8');
  inputMessageStream.on('data', analysisRecvBinaryData);

  verifyClientHandshake(inputMessageStream);
});


function startBroadcastClient() {
  broadcastStream = tls.connect(8443, options, () => {
    process.stdin.pipe(broadcastStream);
    process.stdin.resume();

    // 메시지 수신을 위한 패킷 전송을 수행합니다.
    var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
    broadcastStream.write(message.toString(), 'binary');
    insertNewMessage("브로드캐스트 서버에 연결되었습니다.\n");
  });

  broadcastStream.on('data', (data : Buffer) => {
    var str = Buffer.from(data).toString();
    console.log("Broadcast message received: " +  str.substring(12));

    insertNewMessage(str.substring(12));

    var message : PackingMessage = PackingMessage.ConvertMessage(1, "");
    broadcastStream.write(message.toString(), 'binary');
  });
}


const app : Express = express();

app.use(cors());
app.use(body_parser.urlencoded({ extended: false }));
app.use(body_parser.json());

const server = require('http').createServer(app);


app.get('/', (req : Request, res: Response) => {
  res.send({message: recvMessage}); }
);

app.post('/send', (req: Request, res: Response) => {
  const msg = PackingMessage.ConvertMessage(req.body.command, req.body.data);
  inputMessageStream.write(msg.toString(), 'binary');
  res.send({status: 'ok'});
});


server.listen(9999, () => {
  console.log('API Engine is running on 9999');
});