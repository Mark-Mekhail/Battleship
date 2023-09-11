const express = require('express');
const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');
const { createServer } = require('http');
const { Game, ShipPlacement, boardSize } = require('./game.js');
const usernameGenerator = require('./usernameGenerator.js');

const port = process.env.PORT || 8080;
const maxSpectatingRooms = 1;
const shipSizes = new Map([ ["battleship", 4], ["cruiser", 3], ["submarine", 3], ["destroyer", 2] ]);

// Maps a socket ID to a username.
let usernames = new Map();

// Maps a room number to a game.
let rooms = new Map();

// Maps a socket ID to a WebSocket.
let connections = new Map();

// Maps a socket ID to a set of room numbers that the socket is spectating.
let spectatorWatchLists = new Map();

const maxRoomNumber = 9999;
const freeRooms = [];

// Generate free room numbers.
for (let i = 1000; i <= maxRoomNumber; i++) {
    freeRooms.push(i);
}

// Server setup code modified from https://docs.digitalocean.com/tutorials/app-deploy-websockets/ and ChatGPT
const app = express();

app.use(express.static('client'));

const server = createServer(app);
const wss = new WebSocket.Server({ server });

server.listen(port, () => {
    console.log('Server listening on port ' + port);
});

wss.on("connection", (socket) => {
    // Generate a unique ID for the socket and store it in the connections map.
    const id = uuidv4();

    // Set the socket's username to a random username.
    usernames.set(id, usernameGenerator.generateUsername());
    socket.send(JSON.stringify({ type: "USERNAME_SET", userName: usernames.get(id) }));

    // The room that the socket is currently in. If the socket is not in a room, this is null.
    let room = null;

    connections.set(id, socket);
    spectatorWatchLists.set(id, new Set());

    socket.on("message", (message) => {
        var data;

        // Parse the message as JSON. If it is not valid JSON, send an error message and return.
        try {
            data = JSON.parse(message);
        }
        catch (e) {
            socket.send(JSON.stringify({ type: "ERROR", message: "Invalid JSON." }));
            return;
        }

        // If the message does not have a valid type, send an error message and return.
        switch (data.type) {
            case "SET_USERNAME":
                handleSetUsername(id, socket, room, data);
                break;

            case "CREATE_ROOM":
                room = handleCreateRoom(id, socket, room, data);
                break;

            case "JOIN_ROOM":
                room = handleJoinRoom(id, socket, room, data);
                break;

            case "LEAVE_ROOM":
                room = handleLeaveRoom(id, socket, room, data);
                break;

            case "SPECTATE_ROOM":
                handleSpecatateRoom(id, socket, room, data);
                break;

            case "STOP_SPECTATING_ROOM":
                handleStopSpectatingRoom(id, socket, room, data);
                break;
            
            case "SET_SHIPS":
                handleSetShips(id, socket, room, data);
                break;

            case "FIRE":
                handleFire(id, socket, room, data);
                break;

            case "RESET":
                handleReset(id, socket, room, data);
                break;

            default:
                socket.send(JSON.stringify({ type: "ERROR", message: "Message type missing or invalid." }));
                break;
        }
    });

    socket.on("close", () => {
        // Handle the case where the socket is in a room.
        if (room) {
            let game = getGame(room);
            let playerIndices = getPlayerIndices(game, id);

            if (playerIndices.opponent !== null) {
                // If the opponent is still connected, send a message to the opponent that the player has left.
                connections.get(game.players[playerIndices.opponent]).send(JSON.stringify({ type: "OPPONENT_LEFT" }));
                game.reset("waiting");

                // Remove the player from the game.
                // Make the opponent player 1.
                game.players[0] = game.players[playerIndices.opponent];
                game.players[1] = null;

                for (var spectatorId of game.spectators) {
                    connections.get(spectatorId).send(JSON.stringify({ type: "PLAYER_LEFT", gameInfo: game.getBasicInfo(room, usernames) }));
                }
            }
            else {
                // If the opponent is not connected, remove the room because it is empty.
                for (var spectatorId of game.spectators) {
                    connections.get(spectatorId).send(JSON.stringify({ type: "ROOM_REMOVED", roomId: room }));

                    let watchList = spectatorWatchLists.get(spectatorId)
                    if (watchList.has(room)) {
                        watchList.delete(room);
                    }
                    else {
                        throw new Error("Spectator integrity constraint violation.");
                    }
                }

                rooms.delete(room);
                freeRooms.push(room);
            }
        }

        // Remove the socket from all rooms that it is spectating.
        spectatorWatchLists.get(id).forEach((spectatingRoom) => {
            let game = getGame(spectatingRoom);

            if (!game.spectators.has(id)) {
                throw new Error("Spectator integrity constraint violation.");
            }
            else {
                game.spectators.delete(id);
            }
        });

        // Remove all references to the socket.
        spectatorWatchLists.delete(id);
        usernames.delete(id);
        connections.delete(id);
    });
});

function handleSetUsername(id, socket, room, data) {
    if (!data.username) {
        sendInvalidRequestMessage(socket, "SET_USERNAME");
    }

    if (room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "Cannot change username while in a room." }));
    }
    else {
        if (data.username.length > 20) {
            socket.send(JSON.stringify({ type: "ERROR", message: "Username must be 20 characters or less." }));
            return;
        }

        usernames.set(id, data.username);
        socket.send(JSON.stringify({ type: "USERNAME_SET", username: data.username }));
    }
}

function handleCreateRoom(id, socket, room, data) {
    if (data.isPublic === null || data.isPublic === undefined) {
        sendInvalidRequestMessage(socket, "CREATE_ROOM");
        return room;
    }

    if (room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are already in a room." }));
        return room;
    }

    // Create a new game and add it to the rooms map.
    let game = new Game(id, data.isPublic);
    roomId = getAvailableRoomId();
    rooms.set(roomId, game);

    // Notify the player that the room has been created and that they have joined it.
    socket.send(JSON.stringify({ type: "ROOM_CREATED", roomId: roomId }));
    socket.send(JSON.stringify({ type: "ROOM_JOINED", roomId: roomId }));

    return roomId;
}

function handleJoinRoom(id, socket, room, data) {
    // If the player did not specify a room ID, get the id of a random public room that the user can join.
    var roomId = data.roomId ? data.roomId : getRandomPublicRoomId(true);
    var game = rooms.get(roomId);

    if (room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are already in a room." }));
        return room;
    }
    else if (!game || game.state !== "waiting") {
        socket.send(JSON.stringify({ type: "ERROR", message: "Room is currently full or does not exist." }));
        return room;
    }

    let playerIndices = getPlayerIndices(game, id, false);
    if (playerIndices.player !== null) {
        throw new Error("Player already in game.");
    }

    socket.send(JSON.stringify({ type: "ROOM_JOINED", roomId: data.roomId }));

    // Add the player to the game.
    game.players[1 - playerIndices.opponent] = id;

    if (playerIndices.opponent !== null) {
        game.state = "setup";
        
        for (let i = 0; i < game.players.length; i++) {
            connections.get(game.players[i]).send(JSON.stringify({ type: "START_SETUP", opponentName: usernames.get(game.players[1 - i]) }));
        }

        for (let spectatorId of game.spectators) {
            connections.get(spectatorId).send(JSON.stringify({ type: "STARTING_SETUP", gameInfo: game.getBasicInfo(room, usernames) }));
        }
    }

    return roomId;
}

function handleLeaveRoom(id, socket, room, data) {
    if (!room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are not in a room." }));
        return;
    }

    let game = rooms.get(room);
    let playerIndices = getPlayerIndices(game, id);
    if (playerIndices.player === null) {
        throw new Error("Player not found in room.");
    }

    // Remove the player from the game.
    game.players[playerIndices.player] = null;

    if (playerIndices.opponent !== null) {
        connections.get(game.players[playerIndices.opponent]).send(JSON.stringify({ type: "OPPONENT_LEFT", roomId: room }));
        game.reset("waiting");

        // Make the opponent player 1.
        game.players[0] = game.players[playerIndices.opponent];
        game.players[1] = null;

        for (let spectatorId of game.spectators) {
            connections.get(spectatorId).send(JSON.stringify({ type: "PLAYER_LEFT", gameInfo: game.getBasicInfo(room, usernames) }));
        }
    }
    else {
        // If the opponent is not connected, remove the room because it is empty.
        for (spectatorId of game.spectators) {
            connections.get(spectatorId).send(JSON.stringify({ type: "ROOM_REMOVED", roomId: room }));

            let watchList = spectatorWatchLists.get(spectatorId)
            if (watchList.has(room)) {
                watchList.delete(room);
            }
            else {
                throw new Error("Spectator integrity constraint violation.");
            }
        }

        rooms.delete(room);
        freeRooms.push(room);
    }

    socket.send(JSON.stringify({ type: "ROOM_LEFT", roomId: room }));
    room = null;
}

function handleSpecatateRoom(id, socket, room, data) {
    var roomId = data.roomId ? data.roomId : getRandomPublicRoomId();
    if (!roomId) {
        socket.send(JSON.stringify({ type: "ERROR", message: "No public games in progress." }));
        return;
    }

    var game = rooms.get(roomId);
    var watchList = spectatorWatchLists.get(id);

    if (watchList.size >= maxSpectatingRooms && !watchList.has(roomId)) {
        if (watchList.size > maxSpectatingRooms) {
            throw new Error("Spectator integrity constraint violation.");
        }

        socket.send(JSON.stringify({ type: "ERROR", message: "You are spectating too many rooms." }));
    } 
    else if (!game) {
        socket.send(JSON.stringify({ type: "ERROR", message: "Room " + roomId + " does not exist." }));
    } 
    else if (room === roomId) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You cannot spectate a room you are playing in" }));
    }
    else {
        if (watchList.has(roomId) !== game.spectators.has(id)) {
            throw new Error("Spectators integrity constraint violation.");
        }

        game.spectators.add(id);
        watchList.add(roomId);
        socket.send(JSON.stringify({ type: "SPECTATING_ROOM", gameInfo: game.getBasicInfo(roomId, usernames, true) }));
    }
}

function handleStopSpectatingRoom(id, socket, room, data) {
    if (data.roomId === null || data.roomId === undefined) {
        sendInvalidRequestMessage(socket, "STOP_SPECTATING_ROOM");
        return;
    }

    var game = rooms.get(data.roomId);
    var watchList = spectatorWatchLists.get(id);

    if (!game) {
        if (watchList.has(data.roomId)) {
            throw new Error("Spectators integrity constraint violation.");
        }

        socket.send(JSON.stringify({ type: "ERROR", message: "Room not found." }));
    }
    else if (!watchList.has(data.roomId)) {
        if (game.spectators.has(id)) {
            throw new Error("Spectators integrity constraint violation.");
        }

        socket.send(JSON.stringify({ type: "ERROR", message: "You are not currently spectating room " + data.roomId + "." }));
    }
    else {
        if (!game.spectators.has(id)) {
            throw new Error("Spectators integrity constraint violation.");
        }

        game.spectators.delete(id);
        watchList.delete(data.roomId);
        socket.send(JSON.stringify({ type: "STOPPED_SPECTATING_ROOM", roomId: data.roomId }));
    }
}

function handleSetShips(id, socket, room, data) {
    if (!room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are not in a room." }));
        return;
    }

    if (!data.shipPositions) {
        sendInvalidRequestMessage(socket, "SET_SHIPS");
    }

    let game = getGame(room);
    let playerIndices = getPlayerIndices(game, id);

    if (playerIndices.player === null) {
        throw new Error("Player not found in room.");
    }

    if (game.state !== "setup") {
        socket.send(JSON.stringify({ type: "ERROR", message: "Game has already started." }));
        return;
    }

    // Create a new board and fill it with water.
    var board = new Array(boardSize);
    for (var i = 0; i < boardSize; i++) {
        board[i] = new Array(boardSize);
        board[i].fill("water");
    }

    try {
        // Fill the board with the ships and validate the ship positions.
        game.shipPlacements[playerIndices.player] = fillAndValidateShipPositions(data.shipPositions, board);
    }
    catch (error) {
        socket.send(JSON.stringify({ type: "ERROR", message: error.message }));
        return;
    }

    game.playerBoards[playerIndices.player] = board;
    socket.send(JSON.stringify({ type: "SHIPS_SET", roomId: room }));

    for (var spectatorId of game.spectators) {
        connections.get(spectatorId).send(JSON.stringify({ type: "PLAYER_SET_SHIPS", gameInfo: game.getBasicInfo(room, usernames) }));
    }

    if (game.playerBoards[1 - playerIndices.player]) {
        game.state = "playerOneTurn";

        for (var playerId of game.players) {
            connections.get(playerId).send(JSON.stringify({ type: "START_GAME", yourTurn: playerId === game.players[0] }));
        }

        for (var spectatorId of game.spectators) {
            connections.get(spectatorId).send(JSON.stringify({ type: "START_GAME" }));
        }
    }
}

function handleFire(id, socket, room, data) {
    if (!room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are not in a room." }));
        return;
    }

    if (data.x === null || data.x === undefined || data.y === null || data.y === undefined) {
        sendInvalidRequestMessage(socket, "FIRE");
        return;
    }
    
    let game = getGame(room);
    let playerIndices = getPlayerIndices(game, id);

    if (playerIndices.player === null) {
        throw new Error("Player not found in room.");
    }

    if (game.state !== "playerOneTurn" && game.state !== "playerTwoTurn") {
        socket.send(JSON.stringify({ type: "ERROR", message: "Game has not started." }));
        return;
    }
    else if (game.state === "playerOneTurn" && playerIndices.player !== 0) {
        socket.send(JSON.stringify({ type: "ERROR", message: "It is not your turn." }));
        return;
    }
    else if(game.state === "playerTwoTurn" && playerIndices.player !== 1) {
        socket.send(JSON.stringify({ type: "ERROR", message: "It is not your turn." }));
        return;
    }

    var board = game.playerBoards[playerIndices.opponent];

    if (data.x < 0 || data.x >= boardSize || data.y < 0 || data.y >= boardSize) {
        socket.send(JSON.stringify({ type: "ERROR", message: "Invalid coordinates." }));
        return;
    }
    else if (game.shots[playerIndices.player][data.y][data.x]) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You have already fired at this location." }));
        return;
    }

    game.shots[playerIndices.player][data.y][data.x] = true;
    const target = board[data.y][data.x];

    const opponentSocket = connections.get(game.players[playerIndices.opponent]);

    let message = { x: data.x, y: data.y, hit: false, sunk: false }

    if (game.state === "playerOneTurn") {
        game.state = "playerTwoTurn";
    }
    else {
        game.state = "playerOneTurn";
    }

    if (target === "water") {
        sendFiredMessages(game, room, socket, opponentSocket, playerIndices.player, target, message);
        return;
    }
    else {
        // Update the ship hits.
        var hits = game.shipHits[playerIndices.player].get(target) + 1;
        game.shipHits[playerIndices.player].set(target, hits);

        message.hit = true;
        
        // Check if the ship has been sunk.
        if (hits === shipSizes.get(target)) {
            game.shipsSunk[playerIndices.player].add(target);

            const sunkShipPosition = game.shipPlacements[playerIndices.opponent].get(target);
            const start = { x: sunkShipPosition.x, y: sunkShipPosition.y };
            var end = { x: sunkShipPosition.x, y: sunkShipPosition.y };

            // Calculate the end coordinates of the ship.
            if (sunkShipPosition.orientation === "horizontal") {
                end.x += shipSizes.get(target) - 1;
            }
            else {
                end.y += shipSizes.get(target) - 1;
            }

            message.sunk = true;
            message.ship = target;
            message.start = start;
            message.end = end;

            sendFiredMessages(game, room, socket, opponentSocket, playerIndices.player, target, message);

            if (game.shipsSunk[playerIndices.player].size === shipSizes.size) {
                game.state = "player" + (playerIndices.player + 1) + "Won";

                socket.send(JSON.stringify({ type: "GAME_WON" }));
                opponentSocket.send(JSON.stringify({ type: "GAME_LOST" }));

                for (var spectatorId of game.spectators) {
                    connections.get(spectatorId).send(JSON.stringify({ type: "GAME_OVER", roomId: room, winner: usernames.get(game.players[playerIndices.player]) }));
                }
            }
        }
        else {
            sendFiredMessages(game, room, socket, opponentSocket, playerIndices.player, target, message);
            return;
        }
    }
}

function sendFiredMessages(game, room, socket, opponentSocket, attackerIndex, target, message) {
    message.type = "FIRED";
    socket.send(JSON.stringify(message));
    message.type = "OPPONENT_FIRED";
    opponentSocket.send(JSON.stringify(message));

    message.type = "PLAYER_FIRED";
    message.attackerIndex = attackerIndex;
    message.gameInfo = game.getBasicInfo(room, usernames);
    if (target !== "water") {
        message.ship = target;
    }
    for (var spectatorId of game.spectators) {
        connections.get(spectatorId).send(JSON.stringify(message));
    }
}

function handleReset(id, socket, room, data) {
    if (!room) {
        socket.send(JSON.stringify({ type: "ERROR", message: "You are not in a room." }));
        return;
    }

    let game = getGame(room);

    if (game.state === "player1Won" || game.state === "player2Won") {
        game.reset("setup");
        socket.send(JSON.stringify({ type: "GAME_RESET" }));

        for (var spectatorId of game.spectators) {
            connections.get(spectatorId).send(JSON.stringify({ type: "GAME_RESET", gameInfo: game.getBasicInfo(room, usernames) }));
        }
    }
    else {
        socket.send(JSON.stringify({ type: "ERROR", message: "Game is not over." }));
    }
}

function sendInvalidRequestMessage(socket, type) {
    socket.send(JSON.stringify({ type: "ERROR", message: "Required parameters missing or invalid for " + type + " request." }));
}

// Returns a random public room id where the opponent is either waiting or not waiting for an opponent.
function getRandomPublicRoomId(waitingForOpponent = false) {
    let roomsArray = Array.from(rooms.keys());
    let publicRooms = roomsArray.filter((id) => rooms.get(id).isPublic && (waitingForOpponent == (rooms.get(id).state === "waiting")));

    if (publicRooms.length === 0) {
        return null;
    }

    return publicRooms[Math.floor(Math.random() * publicRooms.length)];
}

// Returns a random room id that is not already in use.
function getAvailableRoomId() {
    if (freeRooms.length === 0) {
        return null;
    }

    let index = Math.floor(Math.random() * freeRooms.length);
    let roomId = freeRooms[index];
    freeRooms.splice(index, 1);
    return roomId;
}

// Returns the game object for the given room id and throws an error if the room is not found.
function getGame(room) {
    let game = rooms.get(room);

    if (!game) {
        throw new Error("Room not found.");
    }

    return game;
}

// Returns the player and opponent indices for the given player id and room.
function getPlayerIndices(game, playerId, shouldBeInRoom = true) {
    let player = null;
    let opponent = null;

    for (let i = 0; i < 2; i++) {
        if (game.players[i] && game.players[i].toString() === playerId.toString()) {
            if (player) {
                throw new Error("Player found twice in room.");
            }

            player = i;
        }
        else if (game.players[i]) {
            if (opponent) {
                throw new Error("Room is full of other players.");
            }

            opponent = i;
        }
    }

    if (player === null) {
        if (shouldBeInRoom) {
            throw new Error("Player not found in room but should be in it.");
        }
        else if (opponent === null) {
            throw new Error("Room is empty.");
        }
    }

    return { 
        player: player, 
        opponent: opponent 
    };
}


// Fills the shipPositions matrix with the ship positions and validates them.
function fillAndValidateShipPositions(shipPositions, board) {
    if (shipPositions.length != shipSizes.size) {
        throw new Error("Invalid number of ships.");
    }

    var shipPlacements = new Map();
    
    for (let ship of shipPositions) {
        if (!shipSizes.get(ship.type)) {
            throw new Error("Invalid ship type.");
        }

        let start = ship.start;
        let end = ship.end;
        let isVertical = start.x === end.x;
        let isHorizontal = start.y === end.y;

        if (start.x < 0 || start.x > boardSize || end.x < 0 || end.x > boardSize || 
            start.y < 0 || start.y > boardSize || end.y < 0 || end.y > boardSize) {
            throw new Error("A ship is out of bounds.");
        }
        if (!(isVertical || isHorizontal)) {
            throw new Error("A ship is has an invalid orientation (neither vertical nor horizontal).");
        }

        // Make sure the ships are not overlapping.
        let minX = Math.min(start.x, end.x);
        let minY = Math.min(start.y, end.y);
        for (let i = 0; i < shipSizes.get(ship.type); i++) {
            let x = minX + (isVertical ? 0 : i);
            let y = minY + (isHorizontal ? 0 : i);

            if (board[y][x] === "water") {
                board[y][x] = ship.type;
            } 
            else {
                throw new Error("A ship is overlapping with another ship.");
            }
        }

        shipPlacements.set(ship.type, new ShipPlacement(minX, minY, isVertical ? "vertical" : "horizontal"));
    }

    return shipPlacements;
}