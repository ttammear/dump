// on client connect server->client
//      - list of required packages
//      - map assetId
//      - client playerid
//      - dynamic objects that need to be created
//          - assetId
//          - position
//          - rotation
//          - dynamic mode (per frame update, static)
//      - list of input types and their description, suggested default (tag?)
//          - type (key, analog, spherical)
//          - tag (player can assign defaults by tag, ie move_forward, jump etc)
//          - flags 
//              key flags - track state, track positive edge, track negative edge
//              analog flags - track state
//              spherical flags - track state
//          NOTE: further in the future this should be input set and/or extensions for vehicles etc
//      - list of connected players
//          - id
//          - name

// on client connect client->server
//      - name
//      - game client version

// per tick client->server
// frameid
// intput keys + mouselook
// tracked events
//      - input edges (if tracked)

// per tick server->client
// tracked transform updates
// tracked events
//      - new objects
//      - deleted objects
//      - teleported objects (if not dynamic object)

// networked input

// on client connect server sends list of tracked keys and their id
// key tracking modes
//      - track state per frame
//          for movement etc
//      - track positive edge (up->down)
//      - track negative edge (down->up)
//          for actions
//
// key state track data
//      - bitfield of all state tracked keys
//
// NOTE: might also need something like exact mouselook at edge trigger
// key edge track data
//      - 1 byte key id
//      - 1 byte flags (direction)
//
// input types
//      - keys
//      - analog
//      - spherical (mouselook)
//
//
//
// Client negotiation
//  - (check if player is allowed to connect)
//  - check if player name is ok
//  - check if client has all required packages (server sends list, client checks)
//  - check if client has all inputs (let choose missing ones)
//  - (let player preload a point on map)
//  - spawn player
//
//


