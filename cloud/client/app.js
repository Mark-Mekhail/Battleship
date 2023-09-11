// Create a new WebSocket object and specify the server URL
const wsHost = 'ws://' + window.location.host;
// wsHost = 'ws://battleship-ws-yidqiexpaq-uw.a.run.app'
const socket = new WebSocket(wsHost);

const shipSizes = new Map([ ["battleship", 4], ["cruiser", 3], ["submarine", 3], ["destroyer", 2] ]);
const boardSize = 8;

const preferredVoiceURIs = ["Google UK English Male", "Microsoft Ryan Online (Natural) - English (United Kingdom)"];
let voice = null;
const soundEffectIntensity = 0.5;

// Keep track of the current room id
var room = null;

// Set up event listeners for the WebSocket connection
socket.addEventListener("open", function(event) {
	console.log("WebSocket connection established.");
});

socket.addEventListener("message", function(event) {
	let data = JSON.parse(event.data);

	switch(data.type) {
		case "ROOM_REMOVED":
			handleRoomRemoved(data);
			break;

		case "PLAYER_FIRED":
			handlePlayerFired(data);
			break;

		case "SPECTATING_ROOM":
			handleSpectatingRoom(data);
			break;

		case "STARTING_SETUP":
			handleStartingSetup(data);
			break;

		case "PLAYER_SET_SHIPS":
			handlePlayerSetShips(data);
			break;

		case "GAME_OVER":
			handleGameOver(data);
			break;

		case "PLAYER_LEFT":
			handlePlayerLeft(data);
			break;

		case "GAME_RESET":
			handleGameReset(data);
			break;

		case "ERROR":
			handleErrorMessage(data);
			break;
		
		default:
			console.log("Message received: " + event.data);
			break;
	}
});

socket.addEventListener("close", function(event) {
 	console.log("WebSocket connection closed.");
});

window.addEventListener("load", function(event) {
	console.log("All resources finished loading!");

	// Make sure the voice is loaded before speaking
	var utterance = new SpeechSynthesisUtterance("");
	this.speechSynthesis.speak(utterance);

  	renderHomeScreen();
});

window.speechSynthesis.onvoiceschanged = function() {
	for (let voiceURI of preferredVoiceURIs) {
		voice = speechSynthesis.getVoices().find((voice) => voice.voiceURI === voiceURI);
		if (voice) {
			break;
		}
	}

	if (!voice) {
		voice = speechSynthesis.getVoices()[0];
	}
}

function handleRoomRemoved(data) {
	if (data.roomId === room) {
		room = null;
		renderHomeScreen();
	}
}

function handlePlayerFired(data) {
	// Consider redrawing all shots, if needed
	updateBoard(1 - data.attackerIndex, data.x, data.y, data.hit);

	if (data.sunk) {
		playShipSunk(data);
	}
	else if (data.hit) {
		playShipHit(data);
	}
	else {
		playMiss(data);
	}
}

function playShipSunk(data) {
	let sinkSound = new Audio("audio/sink.wav");
	sinkSound.volume = soundEffectIntensity * 0.25;
	sinkSound.play();
	setTimeout(() => sinkSound.pause(), 5000);

	resetSpeech();

	speechSynthesis.pause();
	speechSynthesis.cancel();

	var attacker = "Player " + (data.attackerIndex + 1);
	var victim = "player " + (2 - data.attackerIndex);

	var utterance = new SpeechSynthesisUtterance(attacker + " sunk " + victim + "'s " + data.ship + "!");
	utterance.voice = voice;

	speechSynthesis.speak(utterance);
	sinkSound.play();
}

function playShipHit(data) {
	let hitSound = new Audio("audio/hit.wav");
	hitSound.volume = soundEffectIntensity * 0.25;
	hitSound.play();
	setTimeout(() => hitSound.pause(), 2000);

	resetSpeech();

	var attacker = "Player " + (data.attackerIndex + 1);
	var victim = "player " + (2 - data.attackerIndex);

	var utterance = new SpeechSynthesisUtterance(attacker + " hit a " + victim + " ship!");
	utterance.voice = voice;

	hitSound.play();
	speechSynthesis.speak(utterance);
}

function playMiss(data) {
	let missSound = new Audio("audio/miss.wav");
	missSound.volume = soundEffectIntensity * 0.25;
	missSound.play();
	setTimeout(() => missSound.pause(), 2000);
	
	resetSpeech();

	var attacker = "Player " + (data.attackerIndex + 1);

	var utterance = new SpeechSynthesisUtterance(attacker + "missed.");
	utterance.voice = voice;

	speechSynthesis.speak(utterance);
}

function resetSpeech() {
	speechSynthesis.pause();
	speechSynthesis.cancel();
}

function handleSpectatingRoom(data) {
	room = data.gameInfo.roomId;
	renderBattleshipGame(data.gameInfo);
}

function handleStartingSetup(data) {
	renderBattleshipGame(data.gameInfo);
}

function handlePlayerSetShips(data) {
	renderBattleshipGame(data.gameInfo);
}

function handleGameOver(data) {
	speechSynthesis.pause();
	speechSynthesis.cancel();

	var utterance = new SpeechSynthesisUtterance("Game over. " + data.winner + " won!");
	utterance.voice = voice;

	speechSynthesis.speak(utterance);

	showGameOver(data.winner);
}

function handlePlayerLeft(data) {
	renderBattleshipGame(data.gameInfo);
}

function handleGameReset(data) {
	renderBattleshipGame(data.gameInfo);
}

function handleErrorMessage(data) {
	if (data.message) {
		console.log("ERROR: " + data.message)
		const message = document.getElementById('message');
		message.textContent = "Error: " + data.message;
	}
}

// Helper function to remove all children of game div
function clearGameDiv() {
	const gameDiv = document.getElementById('game');
	
	while (gameDiv.firstChild) {
		gameDiv.removeChild(gameDiv.firstChild);
	}

	return gameDiv;
}

function renderHomeScreen() {
	room = null;

	const gameSelectionContainer = document.createElement('div');
	gameSelectionContainer.id = 'game-selection-container';

	const form = document.createElement('form');
	form.id = 'game-selection-form';

	const gameCodeInput = document.createElement('input');
	gameCodeInput.id = 'game-code-input';
	gameCodeInput.type = 'text';
	gameCodeInput.minLength = 4;
	gameCodeInput.maxLength = 4;
	gameCodeInput.pattern = '[0-9]*';
	gameCodeInput.placeholder = '4-digit Game Code';

	const watchGameBtn = document.createElement('button');
	watchGameBtn.type = 'submit';
	watchGameBtn.id = 'watch-game-button';
	watchGameBtn.textContent = 'Watch Game';

	const message = document.createElement('p');
	message.id = 'message';
	message.textContent = 'Leave blank for a random game.';

	// Add real-time validation messages to the game code input
	gameCodeInput.addEventListener('input', (event) => {
		if (event.target.validity.patternMismatch) {
			message.textContent = 'Error: Game code must contain only digits.';
		}
		else {
			message.textContent = 'Leave blank for a random game.';
		}
	});

	form.appendChild(gameCodeInput);
	form.appendChild(watchGameBtn);
	form.addEventListener('submit', (event) => {
		// Prevent the form from refreshing the page
		event.preventDefault();
		socket.send(JSON.stringify({type: "SPECTATE_ROOM", roomId: parseInt(gameCodeInput.value)}));
	});

	gameSelectionContainer.appendChild(form);
	gameSelectionContainer.appendChild(message);

	const gameDiv = clearGameDiv();
	gameDiv.appendChild(gameSelectionContainer);
}

function showGameOver(winner) {
	const gameOverContainer = document.getElementById('game-over-container');
	gameOverContainer.style.display = 'block';
	
	const gameOverMessage = document.getElementById('game-over-message');
	gameOverMessage.textContent = 'GAME OVER \n ' + winner + ' WON!';
}

// Render the Battleship game
function renderBattleshipGame(gameInfo) {
	const gameOverMessage = document.createElement('h2');
	gameOverMessage.id = 'game-over-message';

	const gameOverContainer = document.createElement('div');
	gameOverContainer.id = 'game-over-container';

	if (gameInfo.state !== 'player1Won' && gameInfo.state !== 'player2Won') {
		gameOverContainer.style.display = 'none';
	}
	else {
		winner = (gameInfo.state === 'player1Won') ? gameInfo.players[0] : gameInfo.players[1];
		gameOverMessage.textContent = 'GAME OVER \n' + winner + ' WON!';
		gameOverContainer.style.display = 'block';
	}

	gameOverContainer.appendChild(gameOverMessage);

	const gameContainer = document.createElement('div');
	gameContainer.id = 'game-container';
	gameContainer.appendChild(gameOverContainer);

	getPlayerInfoElements(gameInfo).forEach((element) => {
		gameContainer.appendChild(element);
	});
	
	const actionsContainer = document.createElement('div');
	actionsContainer.id = 'actions-container';
	actionsContainer.appendChild(getMenuButtonElement());

	// Clear the current content of the game div and add the game container
	const gameDiv = clearGameDiv();
	gameDiv.appendChild(gameContainer);
	gameDiv.appendChild(actionsContainer);
}

function getMenuButtonElement() {
	const menuButton = document.createElement('button');
	menuButton.id = 'menu-button';
	menuButton.textContent = 'Back to Main Menu';

	// When the menu button is clicked, send a message to the server to stop spectating the room and return to home screen
	menuButton.addEventListener('click', () => {
		socket.send(JSON.stringify({type: "STOP_SPECTATING_ROOM", roomId: room}));
		renderHomeScreen();
	});

	return menuButton;
}

function getPlayerInfoElements(gameInfo) {
	elements = [];

	// Render the player info for each player
	for (let i = 0; i < 2; i++) {
		const playerInfo = document.createElement('div');
		playerInfo.className = 'player-info';

		const shipPlacements = gameInfo.shipPlacements[i];

		if (gameInfo.players[i] === null) {
			// If the player name is null, the player has not joined the room yet. Display a message to the user.
			const waiting = document.createElement('h2');
			waiting.textContent = "Waiting for another player to join...";
			waiting.classList.add('waiting');
			playerInfo.appendChild(waiting);
		}
		else if (shipPlacements === null) {
			// If the ship placements are null, the player has not set up their board yet. Display a message to the user.
			const waiting = document.createElement('h2');
			waiting.textContent = "Waiting for " + gameInfo.players[i] + " to set up their board...";
			waiting.classList.add('waiting');
			playerInfo.appendChild(waiting);
		}
		else {
			const gameBoard = document.createElement('div');
			gameBoard.className = 'game-board';

			const boardTitle = document.createElement('h2');
			boardTitle.classList.add('board-title');
			boardTitle.textContent = "Player " + (i + 1) + ": " + gameInfo.players[i];

			fillGameBoard(gameBoard, shipPlacements);

			var shotsAgainst = gameInfo.shots ? gameInfo.shots[1 - i] : null;
			if (shotsAgainst) {
				fillShots(gameBoard, shotsAgainst);
			}
			
			playerInfo.appendChild(boardTitle);
			playerInfo.appendChild(gameBoard);
		}
		
		elements.push(playerInfo);
	}

	return elements;
}

// Fill the game board with ships and water
function fillGameBoard(gameBoard, shipPlacements) {
	for (let i = 0; i < boardSize; i++) {
		for (let j = 0; j < boardSize; j++) {
			const cell = document.createElement('canvas');
			cell.ship = false;
			cell.classList.add("cell");
			gameBoard.appendChild(cell);
		}
	}

	addShipsToGameBoard(gameBoard, shipPlacements);
}

// Mark the cells that have been shot at
function fillShots(gameBoard, shotsAgainst) {
	for (let i = 0; i < boardSize; i++) {
		for (let j = 0; j < boardSize; j++) {
			const cell = gameBoard.children[i * boardSize + j];

			if (shotsAgainst[i][j]) {
				drawShot(cell);
			}
		}
	}
}

function addShipsToGameBoard(gameBoard, shipPlacements) {
	for (ship in shipPlacements) {
		const shipPlacement = shipPlacements[ship];
		const size = shipSizes.get(ship);

		for (let i = 0; i < size; i++) {
			var cellPosition = shipPlacement.y * boardSize + shipPlacement.x;

			if (shipPlacement.orientation === "horizontal") {
				cellPosition += i;
			}
			else {
				cellPosition += i * boardSize;
			}

			var orientationClass = shipPlacement.orientation;
			if (i === 0) {
				orientationClass += "-first";
			}
			else if (i === size - 1) {
				orientationClass += "-last";
			}

			const cell = gameBoard.children[cellPosition];
			cell.classList.add(orientationClass);
			cell.id = ship + "-" + i;
			cell.ship = true;
			drawShipCell(cell, shipPlacement.orientation);
		}

	}
}

// Mark a cell as hit or missed
function updateBoard(attackedPlayerIndex, x, y) {
	const gameBoard = document.getElementsByClassName("game-board")[attackedPlayerIndex];
	const cell = gameBoard.children[y * boardSize + x];
	drawShot(cell);
}

// Overlay a translucent red rectangle over the cell to indicate that it has been hit
function drawShot(canvas) {
	const ctx = canvas.getContext("2d");

	if (canvas.ship) {
		ctx.fillStyle = "rgb(255, 0, 0, 0.6)";
	}
	else {
		ctx.fillStyle = "rgb(255, 255, 255, 0.6)";
	}

	ctx.fillRect(0, 0, canvas.width, canvas.height);
}

// Draw a part of a ship on the game board
function drawShipCell(canvas, orientation) {
	switch(canvas.id) {
		case "battleship-0":
			drawBattleship(canvas, 0, orientation);
			break;
		case "battleship-1":
			drawBattleship(canvas, 1, orientation);
			break;
		case "battleship-2":
			drawBattleship(canvas, 2, orientation);
			break;
		case "battleship-3":
			drawBattleship(canvas, 3, orientation);
			break;
		case "cruiser-0":
			drawCruiser(canvas, 0, orientation);
			break;
		case "cruiser-1":
			drawCruiser(canvas, 1, orientation);
			break;
		case "cruiser-2":
			drawCruiser(canvas, 2, orientation);
			break;
		case "submarine-0":
			drawSubmarine(canvas, 0, orientation);
			break;
		case "submarine-1":
			drawSubmarine(canvas, 1, orientation);
			break;
		case "submarine-2":
			drawSubmarine(canvas, 2, orientation);
			break;
		case "destroyer-0":
			drawDestroyer(canvas, 0, orientation);
			break;
		case "destroyer-1":
			drawDestroyer(canvas, 1, orientation);
			break;
		default:
			throw Error("Invalid ship id: " + canvas.id);
	}
}

function drawDestroyer(canvas, partNumber, orientation) {
	canvas.width = 300;
	canvas.height = 300;
	const ctx = canvas.getContext('2d');
	const translation = partNumber * 300;

	// Rotate the canvas if the ship is horizontal
	orientCanvas(ctx, orientation);
	ctx.translate(0, -translation);

	// Draw the hull of the destroyer
	ctx.fillStyle = 'gray';
	const backRadius = 40;
	ctx.beginPath();
	ctx.moveTo(150, 50);
	ctx.quadraticCurveTo(240, 51, 250, 550 - backRadius);
	ctx.lineTo(50, 550 - backRadius);
	ctx.quadraticCurveTo(60, 51, 150, 50);
	ctx.closePath();
	ctx.fill();
	ctx.beginPath();
	ctx.moveTo(50, 550 - backRadius);
	ctx.arcTo(50 + backRadius, 550 - backRadius, 50 + backRadius, 550, backRadius);
	ctx.lineTo(250 - backRadius, 550);
	ctx.arcTo(250 - backRadius, 550 - backRadius, 250, 550 - backRadius, backRadius);
	ctx.closePath();
	ctx.fill();

	// Draw gun
	const barrelLength = 50;
	const barrelWidth = 20;
	const barrelRadius = 8;
	const gunWidth = 80;
	const gunDistFromEnd = 120;
	ctx.fillStyle = 'rgb(60, 60, 60)';

	drawRectRoundedCorners(ctx, 150 - gunWidth / 2, gunDistFromEnd, gunWidth, gunWidth / 2, gunWidth / 8)
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - gunWidth / 5 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 + gunWidth / 5 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();

	// Draw the cabin
	const cabinHeight = 240;
	const cabinWidth = 100;
	ctx.fillStyle = 'rgb(60, 60, 60)';
	drawRectRoundedCorners(ctx, 150 - cabinWidth / 2, 550 - backRadius - cabinHeight, cabinWidth, cabinHeight, cabinWidth / 8);
	ctx.fill();

	// Draw cabin window
	const windowWidth = 50;
	const windowHeight = 20;
	const windowOffset = 10;
	ctx.fillStyle = 'lightblue';
	drawRectRoundedCorners(ctx, 150 - windowWidth / 2, 550 - backRadius - cabinHeight + windowOffset, windowWidth, windowHeight, windowWidth / 8);
	ctx.fill();

	// Reset the canvas origin
	ctx.resetTransform();
}

function drawCruiser(canvas, partNumber, orientation) {
	canvas.width = 300;
	canvas.height = 300;
	const ctx = canvas.getContext('2d');
	const translation = partNumber * 300;

	// Rotate the canvas if the ship is horizontal
	orientCanvas(ctx, orientation);
	ctx.translate(0, -translation);

	// Draw the hull of the cruiser
	ctx.fillStyle = 'gray';
	const hullHeight = 800;
	const hullWidth = 200;
	ctx.beginPath();
	ctx.ellipse(150, 450, hullWidth / 2, hullHeight / 2, 0, 0, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	// Draw the first cabin
	var cabinHeight = 300;
	var cabinWidth = 120;
	var subCabinWidth = cabinWidth * 1.25;
	var offsetFromCenter = -40;
	ctx.fillStyle = 'rgb(60, 60, 60)';
	ctx.beginPath();
	ctx.ellipse(150, 550 + offsetFromCenter - cabinHeight / 2, cabinWidth / 2, cabinHeight / 2, 0, Math.PI, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - subCabinWidth / 2, 550 + offsetFromCenter - cabinHeight / 2, subCabinWidth, cabinHeight / 4, cabinWidth / 8);
	ctx.fill();

	// Draw the window
	const windowWidth = 60;
	const windowHeight = 100;
	const windowOffset = -100;
	ctx.fillStyle = 'lightblue';
	ctx.beginPath();
	ctx.ellipse(150, 550 + offsetFromCenter + windowOffset - cabinHeight / 2 + windowHeight / 2, windowWidth / 2, windowHeight / 2, 0, Math.PI, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	// Draw the second cabin
	cabinHeight = 240;
	cabinWidth = 120;
	offsetFromCenter = 140;
	ctx.fillStyle = 'rgb(60, 60, 60)';
	ctx.beginPath();
	ctx.ellipse(150, 550 + offsetFromCenter - cabinHeight / 2, cabinWidth / 2, cabinHeight / 2, 0, 0, Math.PI);
	ctx.closePath();
	ctx.fill();
	ctx.beginPath();
	ctx.moveTo(150 - cabinWidth / 4, 550 + offsetFromCenter - cabinHeight / 2);
	ctx.lineTo(150 - cabinWidth / 8, 550 + offsetFromCenter - cabinHeight * 0.6);
	ctx.lineTo(150 + cabinWidth / 8, 550 + offsetFromCenter - cabinHeight * 0.6);
	ctx.lineTo(150 + cabinWidth / 4, 550 + offsetFromCenter - cabinHeight / 2);
	ctx.closePath();
	ctx.fill();

	// Draw guns
	const barrelLength = 40;
	const barrelWidth = 16;
	const barrelRadius = 6;
	const gunWidth = 60;
	const gunDistFromEnd = 100;
	ctx.fillStyle = 'rgb(60, 60, 60)';

	drawRectRoundedCorners(ctx, 150 - gunWidth / 2, gunDistFromEnd, gunWidth, gunWidth / 2, gunWidth / 8)
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - gunWidth / 5 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 + gunWidth / 5 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();

	// Draw middle turrets
	ctx.translate(150, 0);
	ctx.translate(70, 500);
	ctx.rotate(Math.PI * 0.3);
	ctx.fill();
	drawCruiserGun(ctx);
	ctx.rotate(-Math.PI * 0.3);

	ctx.translate(-140, 0);
	ctx.rotate(-Math.PI * 0.3);
	drawCruiserGun(ctx);
	ctx.fill();
	ctx.rotate(Math.PI * 0.3);
	ctx.translate(70, 0);

	// Draw rear turrets
	ctx.translate(0, 300);
	ctx.translate(35, 0);
	ctx.rotate(Math.PI * 0.8);
	ctx.fill();
	drawCruiserGun(ctx);

	ctx.rotate(-Math.PI * 0.8);
	ctx.translate(-70, 0);
	ctx.rotate(-Math.PI * 0.8);
	drawCruiserGun(ctx);
	ctx.fill();

	ctx.rotate(Math.PI * 0.8);
	ctx.translate(-115, -800);

	// Reset the canvas origin
	ctx.resetTransform();
}

function drawCruiserGun(ctx) {
	const barrelLength = 30;
	const barrelWidth = 16;
	const barrelRadius = 6;
	const gunWidth = 60;
	ctx.fillStyle = 'rgb(60, 60, 60)';

	drawRectRoundedCorners(ctx, -gunWidth / 2, 0, gunWidth, gunWidth / 2, gunWidth / 8)
	ctx.fill();
	drawRectRoundedCorners(ctx, -barrelWidth / 2, -barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();
}

function drawSubmarine(canvas, partNumber, orientation) {
	canvas.width = 300;
	canvas.height = 300;
	const ctx = canvas.getContext('2d');
	const translation = partNumber * 300;

	// Rotate the canvas if the ship is horizontal
	orientCanvas(ctx, orientation);
	ctx.translate(0, -translation);

	// Draw the hull of the submarine
	ctx.fillStyle = 'rgb(180, 180, 0)';
	const hullHeight = 800;
	const hullWidth = 200;
	ctx.beginPath();
	ctx.ellipse(150, 450, hullWidth / 2, hullHeight / 2, 0, 0, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	// Draw the cabin
	const cabinHeight = 180;
	const cabinWidth = 120;
	ctx.fillStyle = 'rgb(60, 60, 60)';
	ctx.beginPath();
	ctx.ellipse(150, 600, cabinWidth / 2, cabinHeight / 2, 0, 0, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	// Draw top things
	const firstRadius = 25;
	const secondRadius = 35;
	const offsetFromCabin = 125;
	ctx.fillStyle = 'gray';
	ctx.beginPath();
	ctx.ellipse(150, 600 + cabinHeight / 2 - offsetFromCabin, firstRadius, firstRadius, 0, 0, 2 * Math.PI);
	ctx.ellipse(150, 600 + cabinHeight / 2 - offsetFromCabin + firstRadius + secondRadius + 5, secondRadius, secondRadius, 0, 0, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	// Draw the gun thing
	const barrelLength = 40;
	const barrelWidth = 30;
	const barrelRadius = 8;
	const gunRadius = 40;
	const gunDistFromEnd = 200;
	ctx.fillStyle = 'rgb(60, 60, 60)';

	drawRectRoundedCorners(ctx, 150 - gunRadius, gunDistFromEnd, gunRadius * 2, gunRadius * 2.5, 20);
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();

	// Reset the canvas origin
	ctx.resetTransform();
}

function drawBattleship(canvas, partNumber, orientation) {
	canvas.width = 300;
	canvas.height = 300;
	const ctx = canvas.getContext('2d');
	const translation = partNumber * 300;

	// Rotate the canvas if the ship is horizontal
	orientCanvas(ctx, orientation);
	ctx.translate(0, -translation);

	// Draw the hull of the battleship
	ctx.fillStyle = 'gray';
	ctx.beginPath();
	ctx.moveTo(150, 50);
	ctx.quadraticCurveTo(250, 100, 250, 600);
	ctx.quadraticCurveTo(250, 1150, 150, 1150);
	ctx.quadraticCurveTo(50, 1150, 50, 600);
	ctx.quadraticCurveTo(50, 100, 150, 50);
	ctx.closePath();
	ctx.fill();

	// Draw guns
	const barrelLength = 60;
	const barrelWidth = 20;
	const barrelRadius = 8;
	const gunRadius = 50;
	const gunDistFromEnd = 240;
	ctx.fillStyle = 'rgb(60, 60, 60)';

	// Draw the top gun
	ctx.beginPath();
	ctx.moveTo(150, gunDistFromEnd);
	ctx.ellipse(150, gunDistFromEnd, gunRadius, gunRadius * 1.25, 0, 0, Math.PI);
	ctx.closePath();
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - gunRadius / 2 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 + gunRadius / 2 - barrelWidth / 2, gunDistFromEnd - barrelLength, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();

	// Draw the bottom gun
	ctx.beginPath();
	ctx.moveTo(150, 1200 - gunDistFromEnd);
	ctx.ellipse(150, 1200 - gunDistFromEnd, gunRadius, gunRadius * 1.25, 0, Math.PI, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 - gunRadius / 2 - barrelWidth / 2, 1200 - gunDistFromEnd - 10, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();
	drawRectRoundedCorners(ctx, 150 + gunRadius / 2 - barrelWidth / 2, 1200 - gunDistFromEnd - 10, barrelWidth, barrelLength + barrelRadius, barrelRadius);
	ctx.fill();

	// Draw lower cabin
	let cabinDistFromEnd = gunDistFromEnd + gunRadius + 50;
	let cabinWidth = 120;
	let cabinCornerRadius = 60;
	ctx.fillStyle = 'rgb(60, 60, 60)';
	drawRectRoundedCorners(ctx, 150 - cabinWidth / 2, cabinDistFromEnd, cabinWidth, 1200 - cabinDistFromEnd * 2, cabinCornerRadius);
	ctx.fill();

	// Draw upper cabin
	const cabinStart = cabinDistFromEnd + 90;
	const cabinEnd = 1200 - cabinDistFromEnd - 30;
	cabinWidth *= 0.6;
	cabinCornerRadius *= 0.6;

	const windowOffsetFromUpperCabin = -5;
	ctx.fillStyle = 'lightblue';
	ctx.beginPath();
	ctx.ellipse(150, cabinStart - windowOffsetFromUpperCabin, cabinWidth * 0.4, 40, 0, 0, 2 * Math.PI);
	ctx.closePath();
	ctx.fill();

	ctx.fillStyle = 'gray';
	drawRectRoundedCorners(ctx, 150 - cabinWidth / 2, cabinStart, cabinWidth, cabinEnd - cabinStart, cabinCornerRadius);
	ctx.fill();

	// Reset the canvas origin
	ctx.resetTransform();
}

// Courtesy of chatGPT
function drawRectRoundedCorners(ctx, x, y, width, height, radius) {
	ctx.beginPath();
	ctx.moveTo(x + radius, y);
	ctx.lineTo(x + width - radius, y);
	ctx.arcTo(x + width, y, x + width, y + radius, radius);
	ctx.lineTo(x + width, y + height - radius);
	ctx.arcTo(x + width, y + height, x + width - radius, y + height, radius);
	ctx.lineTo(x + radius, y + height);
	ctx.arcTo(x, y + height, x, y + height - radius, radius);
	ctx.lineTo(x, y + radius);
	ctx.arcTo(x, y, x + radius, y, radius);
	ctx.closePath();
}

function orientCanvas(ctx, orientation) {
	if (orientation === 'horizontal') {
		ctx.translate(150, 150);
		ctx.rotate(3 * Math.PI / 2);
		ctx.translate(-150, -150);
	}
}

function addShadowing(ctx, shadowOffset, shadowBlur, shadowColor) {
	ctx.shadowOffsetY = shadowOffset;
	ctx.shadowOffsetX = shadowOffset;
	ctx.shadowBlur = shadowBlur;
	ctx.shadowColor = shadowColor;
}

function removeShadowing(ctx) {
	ctx.shadowOffsetY = 0;
	ctx.shadowOffsetX = 0;
	ctx.shadowBlur = 0;
}