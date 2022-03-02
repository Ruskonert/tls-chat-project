import express, { Express, Request, Response } from 'express';
import path from 'path';
import cors from 'cors';

const app : Express = express();
const server = require('http').createServer(app);

app.use(cors());
app.get('/', (req : Request, res: Response) => {
  res.send({message: 'hello'});
});

server.listen(3002, () => {
  console.log('server is running on 3002');
});
