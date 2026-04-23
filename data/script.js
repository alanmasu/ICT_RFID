
function onLoad(){
    console.log("message from body");
    reflesh();
    setInterval(reflesh, 1000);
}
function buttonSetVal(bnt) {
    if(bnt.dataset.state == "0"){
        bnt.value = "ON";
    }else{
        bnt.value = "OFF";
    }
}

function reflesh(){
    let rec = new XMLHttpRequest();
    rec.open('GET', "/mainPageEndpoint");
    rec.send();
    rec.onload = () => {
        if(rec.responseType == ""){
            //console.log(rec.responseText);
            let jsonObj = JSON.parse(rec.responseText);
            //console.log('rec.responseText', rec.responseText)
            let ledList = jsonObj.leds;
            //console.log('ledList', ledList)
            let buttons = document.querySelectorAll(".led-button");
            let labels = document.querySelectorAll(".led-label");
            if(ledList.length == buttons.length && ledList.length == labels.length){
                for(let i = 0; i < ledList.length; ++i){
                    let led = ledList[i];
                    for(let j = 0; j < buttons.length; ++j){
                        if(ledList[i].pin == buttons[j].dataset.pin){
                            let button = buttons[j];
                            button.dataset.state = led.state;
                            buttonSetVal(button);
                            break;
                        }
                    }
                    for (let j = 0; j < labels.length; j++) {
                        if(ledList[i].pin == labels[j].dataset.pin){
                            let label = labels[j];
                            label.innerText = led.state == '1' ? 'ON' : 'OFF';
                        }
                    }
                }
            }
        }else{
            console.error("error type");
            console.log("type:", rec.responseType);
            console.log("response:", rec.response);
            console.log("responseText:", rec.responseText);
        }
    };
}

function buttonClick(element){
    //Modifico me stesso
    element.dataset.state = element.dataset.state == '1' ? '0': '1' ;
    let rec = new XMLHttpRequest();
    let list = [];
    let msg = "";
    let buttons = document.querySelectorAll(".led-button");
    if(buttons !== undefined){
        console.log("buttons", buttons);
        if(buttons.length > 0){
            buttons.forEach(function(btn){
                list.push({pin: btn.dataset.pin, state: btn.dataset.state});
            });
        }
    }
    console.log("list", list);
    let jsonObj = {leds: list};
    msg = JSON.stringify(jsonObj);
    rec.open('POST', "/ledState", true);
    rec.setRequestHeader('Content-type', 'application/json');
    rec.send(msg);
}

function saveConfig(element){
    let rec = new XMLHttpRequest();
    rec.open('GET', "/saveConfig");
    rec.send();
    // window.location.href = "/saveConfig";
}