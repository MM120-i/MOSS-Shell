const term = new Terminal({
    cursorBlink: true,
    fontSize: 15,
    fontFamily: "'Cascadia Code', 'JetBrains Mono', 'Fira Code', 'SF Mono', 'Consolas', monospace",
    fontWeight: 400,
    letterSpacing: 0,
    lineHeight: 1.3,
    theme: {
        background: '#0a0a1a',
        foreground: '#b3ffe0',
        cursor: '#00f0ff',
        cursorAccent: '#0a0a1a',
        selectionBackground: 'rgba(0,240,255,0.25)',
        selectionForeground: '#ffffff',
        black: '#0d0d22',
        red: '#ff2e97',
        green: '#00ff66',
        yellow: '#ffdd00',
        blue: '#00c8ff',
        magenta: '#c74ded',
        cyan: '#00f0ff',
        white: '#ddeeff',
        brightBlack: '#445577',
        brightRed: '#ff5eaa',
        brightGreen: '#44ff88',
        brightYellow: '#ffee44',
        brightBlue: '#44ddff',
        brightMagenta: '#e088ff',
        brightCyan: '#66ffff',
        brightWhite: '#ffffff'
    },
    allowProposedApi: true
});

const fitAddon = new FitAddon.FitAddon();
term.loadAddon(fitAddon);
term.open(document.getElementById('term'));

const ws = new WebSocket('ws://' + location.host + '/ws');

const sendSize = () => {
    fitAddon.fit();
    ws.send(JSON.stringify({ cols: term.cols, rows: term.rows }));
    document.getElementById('dims').textContent = term.cols + 'x' + term.rows;
};

ws.onopen = () => {
    document.getElementById('status').setAttribute('data-state', 'live');
    sendSize();
    term.focus();
};

ws.onmessage = e => term.write(e.data);
ws.onclose = () => {
    document.getElementById('status').setAttribute('data-state', 'dead');
};

term.onData(d => ws.send(d));
window.addEventListener('resize', sendSize);
