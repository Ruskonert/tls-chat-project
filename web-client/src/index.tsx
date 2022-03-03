import axios from "axios";
import { Fragment, useEffect } from 'react';
import ReactDOM from 'react-dom';
import * as net from 'net';

import './index.css';

async function renderChatMessage() {
    await axios.get("http://localhost:9999/").then(res => {
        var chatMessage : string[] = res.data.message;
        const ChatContainer = (
            <Fragment>
                <div className="desc">채팅 창</div>
                <hr />
                {chatMessage.map(item => 
                <div className="chat-content">
                    <p>{item}</p>
                </div>
                )}
                <script>
                    objDiv = document.getElementById('chat-container');
                    objDiv.scrollTop = objDiv.scrollHeight;
                </script>
            </Fragment>
        );
        ReactDOM.render(ChatContainer, document.getElementById('chat-container'));
    });
}

setInterval(renderChatMessage, 1000);