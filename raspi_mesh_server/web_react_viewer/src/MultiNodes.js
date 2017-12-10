import React,{Component} from 'react';
import Websocket from 'react-websocket';
import merge from 'deepmerge';
import NodeView from './NodeView';

import "./MultiNodes.css"

function mergeSensors(map_old,map_new){
    function overwriteMerge(destinationArray, sourceArray, options) {
        return sourceArray
    }
    //Overwrite required in order not to grow arrays (verified)
    let result = merge(map_old,map_new,{arrayMerge:overwriteMerge});
    return result;
}

function NodesMap(props){
    const listitems = Object.keys(props.updatemap).map((node) =>
        <NodeView   key={node} 
                    //nodeName={node} 
                    nodeName={props.nodesinfo[node]?props.nodesinfo[node].name:"Node "+node} 
                    sensors={props.updatemap[node]}/>
    );
    return(
        <div className="NodesMap">{listitems}</div>
    );

}

class MultiNodes extends Component{
    constructor(props) {
        super(props);
        this.state = {
            updatemap: {},
            nodesinfo:{}
        }
    }
    requestNodesInfo(){
		var jReq = {
            request : {
                id 		: Math.floor(Math.random() * 10000),
                type : "nodesinfo"
            }
        };
        var tReq = JSON.stringify(jReq);
        this.refWebSocket.state.ws.send(tReq);
		var jReqUpdate = {
            request : {
                id 		: Math.floor(Math.random() * 10000),
                type : "update"
            }
        };
        var tReqUpdate = JSON.stringify(jReqUpdate);
        this.refWebSocket.state.ws.send(tReqUpdate);
    }
    morefunc(){
        console.log("morefunc()",this);
    }
    websocketOpen() {
        //too early still connecting
        console.log("this from websocketOpen() ",this);
        this.morefunc();
        this.requestNodesInfo();
        //setTimeout(()=>{this.requestNodesInfo();}, 3000);
      }
      
    handleData(data) {
        let server_message = JSON.parse(data);
        console.log("server_message: ",server_message);
        if(server_message.update)
        {
            this.setState( (prevState,props) => (
                {
                    updatemap: mergeSensors(prevState.updatemap,server_message.update)
                }
            )
            );
        }
        else if(server_message.response.nodesinfo)
        {
            this.setState({nodesinfo:server_message.response.nodesinfo});
            console.log("nodesinfo: ",server_message.response.nodesinfo);
        }
    }
    render(){
    return(
        <div>
            <hr/>
            <NodesMap   updatemap={this.state.updatemap}
                        nodesinfo={this.state.nodesinfo}/>
            <hr/>
            <Websocket  url='ws://10.0.0.12:4348/measures'
                onMessage={this.handleData.bind(this)}
                onOpen={this.websocketOpen.bind(this)}
                debug={true}
                ref={(websref) => {this.refWebSocket = websref;}  }
            />
        </div>
        );
    }
}


export default MultiNodes;
