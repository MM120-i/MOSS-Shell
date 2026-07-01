const term = new Terminal({
    cursorBlink:true,
    fontSize:14,
    theme: {
        background:'#1e1e2e',
        foreground:'#cdd6f4',
        cursor:'#f5e0dc',
        selectionBackground:'#585b70'
    }
});

const fitAddon = new FitAddon.FitAddon();

term.loadAddon(fitAddon);
term.open(document.getElementById('term'));

const ws = new WebSocket('ws://'+location.host+'/ws');

const sendSize = () => {
    fitAddon.fit();
    ws.send(JSON.stringify({
        cols:term.cols,
        rows:term.rows
    }));
}

ws.onopen = () =>{
    sendSize();
    term.focus()
};

ws.onmessage = e => term.write(e.data);
term.onData(d => ws.send(d));
window.addEventListener('resize',sendSize);