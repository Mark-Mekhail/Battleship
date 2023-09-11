const boardSize = 8;

class Game {
    constructor(playerOneId, isPublic) {
        // players is an array of two player IDs. If a player has not joined, the corresponding index is null.
        this.isPublic = isPublic;
        this.players = [playerOneId, null];
        this.spectators = new Set();
        this.reset("waiting");
    }

    reset(gameState) {
        // playerBoards is an array of two matrices of strings that represents a player's board setup.
        this.playerBoards = [null, null];
        // shipPlacements is an array of two maps of ship placements. The first map tracks the first player's ship placements; the second map tracks the second player's placements.
        this.shipPlacements = [null, null];
        // shots is an array of two matrices of booleans that track a player's shots.
        this.shots = [new Array(boardSize), new Array(boardSize)];
        for (let i = 0; i < boardSize; i++) {
            for (let j = 0; j < 2; j++) {
                this.shots[j][i] = new Array(boardSize);
                this.shots[j][i].fill(false);
            }
        }
        // shipHits is an array of two maps of ship hit counts. The first map tracks the first player's hits against the second player; the second map tracks the reverse.
        this.shipHits = [new Map([ ["battleship", 0], ["cruiser", 0], ["submarine", 0], ["destroyer", 0] ]), 
                         new Map([ ["battleship", 0], ["cruiser", 0], ["submarine", 0], ["destroyer", 0] ])];
        // shipsSunk is an array of two sets of ship names. The first set tracks the ships sunk by the first player; likewise for the second player.
        this.shipsSunk = [new Set(), new Set()];
        // gameState is a string that tracks the current state of the game. It can be either "waiting", "setup", "playerOneTurn", "playerTwoTurn", "player1Won", or "player2Won".
        this.state = gameState;
    }

    getBasicInfo(roomId, usernames, includeShots = false) {
        let info = { 
            roomId: roomId,
            state: this.state, 
            players: this.players.map((playerId) => usernames.get(playerId)),
            shipPlacements: this.shipPlacements.map((placements) => placements == null ? null : Object.fromEntries(placements))
        };

        if (includeShots) {
            info.shots = this.shots;
        }

        return info;
    }
}

// Specified the location of a ship by its top left corner and its orientation.
class ShipPlacement {
    constructor(x, y, orientation) {
        this.x = x;
        this.y = y;
        this.orientation = orientation;
    }
}

module.exports = { Game, ShipPlacement, boardSize };