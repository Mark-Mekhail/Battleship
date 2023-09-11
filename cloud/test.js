const WebSocket = require('ws');

const wsOrigin = 'ws://localhost:8080';
const sockets = [new WebSocket(wsOrigin, { followRedirects: true }), new WebSocket(wsOrigin, { followRedirects: true })];

const boardSize = 8;
const delay = 10000;
var start = false;
var roomId = 0;

// Set up event listeners for the WebSocket connection
sockets[0].addEventListener("open", function(event) {
    console.log("WebSocket connection established.");

    sockets[0].send(JSON.stringify({type: "CREATE_ROOM", isPublic: true}));
});

sockets[0].addEventListener("message", function(event) {
    console.log("Message received:", event.data);
    message = JSON.parse(event.data);

    switch (message.type) {
        case "ROOM_CREATED":
            roomId = message.roomId;
            startTest();
            break;
    }
});
  
sockets[0].addEventListener("close", function(event) {
    console.log("WebSocket connection closed.");
});

sockets[1].addEventListener("open", function(event) {
    console.log("WebSocket connection established.");

    sockets[1].send(JSON.stringify({type: "CREATE_ROOM", isPublic: true}));
});

sockets[1].addEventListener("message", function(event) {
    console.log("Message received:", event.data);

    message = JSON.parse(event.data);

    switch (message.type) {
        case "ROOM_CREATED":
            startTest(100);
            break;
    }
});
  
sockets[1].addEventListener("close", function(event) {
    console.log("WebSocket connection closed.");
});

function startTest() {
    if (!start) {
        start = true;
        return;
    }

    var promise = sendMessage(1, {type: "LEAVE_ROOM"}, 100)
    .then(() => {
        return sendMessage(1, {type: "JOIN_ROOM", roomId: roomId}, 100);
    })

    promise = promise.then(() => {
        return sendMessage(0, {type: "RESET"}, delay);
    }).then(() => {
        return sendMessage(0, {type: "SET_SHIPS", shipPositions: getTestShipPositions()}, delay);
    }).then(() => {
        return sendMessage(1, {type: "SET_SHIPS", shipPositions: getTestShipPositions()}, delay);
    });

    for (var i = 0; i < boardSize; i++) {
        for (var j = 0; j < boardSize; j++) {
            const x = i;
            const y = j;
            promise = promise.then(() => {
                return sendMessage(0, {type: "FIRE", x: x, y: y}, delay);
            }).then(() => {
                return sendMessage(1, {type: "FIRE", x: x, y: y}, delay);
            });
        }
    }
    
    setInterval(() => {
        promise = promise.then(() => {
            return sendMessage(0, {type: "RESET"});
        }).then(() => {
            return sendMessage(0, {type: "SET_SHIPS", shipPositions: getTestShipPositions()});
        }).then(() => {
            return sendMessage(1, {type: "SET_SHIPS", shipPositions: getTestShipPositions()});
        });

        for (var i = 0; i < boardSize; i++) {
            for (var j = 0; j < boardSize; j++) {
                const x = i;
                const y = j;
                promise = promise.then(() => {
                    return sendMessage(0, {type: "FIRE", x: x, y: y}, delay);
                }).then(() => {
                    return sendMessage(1, {type: "FIRE", x: x, y: y}, delay);
                });
            }
        }
    }, delay * boardSize * boardSize * 2 + 1000);
}

function sendMessage(socketNum, message, timeout = 1000) {
    return new Promise((resolve, reject) => {
        sockets[socketNum].send(JSON.stringify(message));
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
