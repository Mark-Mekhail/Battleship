const WebSocket = require('ws');

const wsOrigin = 'ws://localhost:8080';
const socket = new WebSocket(wsOrigin, { followRedirects: true });

const boardSize = 8;
const delay = 10000;
var roomId = 1233;

// Set up event listeners for the WebSocket connection
socket.addEventListener("open", function(event) {
    console.log("WebSocket connection established.");

    sendMessage(JSON.stringify({type: "JOIN_ROOM", roomId: roomId}), 500);
});

socket.addEventListener("message", function(event) {
    message = JSON.parse(event.data);
    console.log(message);

    switch (message.type) {
        case "ROOM_JOINED":
            start();
            break;
    }
});
  
socket.addEventListener("close", function(event) {
    console.log("WebSocket connection closed.");
});

function start() {
    var promise =  sendMessage({type: "JOIN_ROOM", roomId: roomId}, 2000);

    promise = promise.then(() => {
        return sendMessage({type: "SET_SHIPS", shipPositions: getTestShipPositions()}, 2000);
    });

    while (true) {
        for (var i = 0; i < boardSize; i++) {
            for (var j = 0; j < boardSize; j++) {
                const x = i;
                const y = j;
                promise = promise.then(() => {
                    return sendMessage({type: "FIRE", x: x, y: y}, delay);
                });
            }
        }
    }
}

function sendMessage(message, timeout = 1000) {
    return new Promise((resolve, reject) => {
        socket.send(JSON.stringify(message));
        setTimeout(resolve, timeout);
    });
}

function getTestShipPositions() {
    positions = [];
    var xSeed = Math.floor(Math.random() * (boardSize - 4));
    var ySeed = Math.floor(Math.random() * (boardSize - 4));
    positions.push({start: {x: xSeed, y: ySeed}, end: {x: xSeed, y: ySeed + 3}, type: "battleship"});
    positions.push({start: {x: xSeed + 1, y: ySeed}, end: {x: xSeed + 1, y: ySeed + 2}, type: "cruiser"});
    positions.push({start: {x: xSeed + 2, y: ySeed}, end: {x: xSeed + 2, y: ySeed + 2}, type: "submarine"});
    positions.push({start: {x: xSeed + 3, y: ySeed}, end: {x: xSeed + 3, y: ySeed + 1}, type: "destroyer"});
    return positions;
}