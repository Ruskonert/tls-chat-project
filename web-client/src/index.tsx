import axios from "axios";
import React, { Fragment, Component } from 'react';
import ReactDOM from 'react-dom';
import './index.css';

import { PackingMessage } from './client_type';

interface Command
{
  name: string,
  command_string: string,
  command_code: number,
}

var   chatMessage : string[] = [];
var   ChatContainer : JSX.Element = (<Fragment><div className="desc">채팅 창</div><hr /></Fragment>);
const command_arr : Command[] = [];

command_arr.push({name: 'CMD_HANDSHAKE', command_string: "", command_code: 0x00});
command_arr.push({name: 'CMD_BROADCASE_MESSAGE', command_string: "", command_code: 0x01});
command_arr.push({name: 'CMD_CHANGE_NICKNAME', command_string: "name", command_code: 0x02});
command_arr.push({name: 'CMD_SECRET_MESSAGE', command_string: "send", command_code: 0x03});
command_arr.push({name: 'CMD_STATUS', command_string: "status", command_code: 0x04});
command_arr.push({name: 'CMD_DISCONNECT', command_string: "exit", command_code: 0x05});
command_arr.push({name: 'CMD_NOT_SUPPORTED', command_string: "", command_code: 0xFF});


function getCommandByCode(code: number) : Command
{
    var cmd : Command = command_arr[command_arr.length - 1];
    command_arr.forEach(c => {
      if(code === c.command_code) {
        cmd = c;
        return false;
      }
    });

    return cmd;
}


function GetCommandByString(name: string) : Command
{
    var cmd : Command = command_arr[command_arr.length - 1]
    command_arr.forEach(c => {
        if(c.command_string.toUpperCase() === name.toUpperCase()) {
            cmd = c;
            return false;
        }});

    return cmd;
}


function applyChatContainerContent(message: string[])
{
    return (
        <Fragment>
            <div className="desc">채팅 창</div>
            <hr />
            {message.map(item => 
            <div className="chat-content">
                <p>{item}</p>
            </div>
            )}
        </Fragment>
    );
}


setInterval(() => {
    ChatContainer = applyChatContainerContent(chatMessage);
    ReactDOM.render(ChatContainer, document.getElementById('chat-container'));
}, 100);



function requestFunction() {
    axios.get('http://localhost:9999/').then(res => {
        var respMessage : string[] = res.data.message;
        const intersection = respMessage.filter(x => !chatMessage.includes(x));
        chatMessage.push(...intersection);
        setTimeout(requestFunction, 0);
    });
}

setTimeout(requestFunction, 1000);



function submitPackingMessage(message?: string)
{
    if(!message || message.length <= 1)
    {
        chatMessage.push("[SYSTEM] 내용을 입력해주세요.");
        return;
    }

    var pMessage : PackingMessage;
    var command : Command;

    if(message.startsWith('/')) {
        var messageSplit = message.split(" ");
        if(messageSplit.length <= 1) {
            command = GetCommandByString(message.substring(1, message.length - 1));
            pMessage = PackingMessage.ConvertMessage(command.command_code, '');
        }
        
        else {
            command = GetCommandByString(messageSplit[0].substring(1));
            delete messageSplit[0];
            var data = messageSplit.join(' ');
            pMessage = PackingMessage.ConvertMessage(command.command_code, data.substring(1, data.length - 1));
        }
    }
    else {
        command = getCommandByCode(0x01);
        pMessage = PackingMessage.ConvertMessage(command.command_code, message.substring(0, message.length - 1));
    }

    axios.post('http://localhost:9999/send',  JSON.stringify(pMessage), {
          headers: {
            "Content-Type": `application/json`,
          },
        })
        .then((res) => {
          console.log(res);
        });
}


interface IMessageComponent {
    message?: string;
}


function onClickSubmitButton(e : React.MouseEvent<HTMLButtonElement>) {
    
}


class ControllerTextBox extends Component<unknown, IMessageComponent>
{
    constructor(props : IMessageComponent) {
        super(props);
        this.state = { message: '' };
        this.handleOnKeyUp = this.handleOnKeyUp.bind(this);
        this.handleOnChange = this.handleOnChange.bind(this);
    }

    handleOnChange(event : React.ChangeEvent<HTMLTextAreaElement>) {
        this.setState({message: event.target.value})
    }

    handleOnKeyUp(event : React.KeyboardEvent<HTMLTextAreaElement>) {
        if(event.key === 'Enter') {
            const message = this.state.message;
            this.setState({message: ''});
            submitPackingMessage(message);
        }
    }

    render(): React.ReactNode {
        return (
            <textarea value={this.state.message} 
            placeholder="명령어 또는 채팅을 입력하세요"
            onChange={this.handleOnChange}
            onKeyUp={this.handleOnKeyUp}
            className="controller-txt-area"></textarea>
        );
    }
}

// 컨트롤러 영역에 대한 앨리먼트를 정의합니다.
const ControllerElement = (
    <Fragment>
        <ControllerTextBox />
        <button onClick={onClickSubmitButton} className="controller-submit-btn">
            <p>명령/메시지 전송</p>
        </button>
    </Fragment>
);

// 기본 스크립트를 작성합니다.
const ScriptPage = (
    <Fragment>
        const _refreshScroll = () =&gt; &#123;
            objDiv = document.getElementById('chat-container');
            objDiv.scrollTop = objDiv.scrollHeight;
        &#125;;
        setInterval(_refreshScroll, 1000);
    </Fragment>
);


ReactDOM.render(ScriptPage, document.getElementById("page-script"));
ReactDOM.render(ControllerElement, document.getElementById("controller"));