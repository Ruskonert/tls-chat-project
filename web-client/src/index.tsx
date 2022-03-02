import React from 'react';
import axios from "axios";
import { useEffect } from 'react';
import ReactDOM from 'react-dom';
import './index.css';

function App2() {
    const callApi = async()=>{
      axios.get("/api").then((res)=>{console.log(res.data.test)});
    };
  
    useEffect(()=>{
      callApi();
    }, []);
    
    return (
      <div className="App">
      ...
      </div>
    );
  }

ReactDOM.render(<App2 />, document.getElementById('root'));