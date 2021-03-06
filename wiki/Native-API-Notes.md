# Magenta Native API

Original (native) engine interfaces

New engine interfaces for other modules oriented on rewritten Valve's module factory  
Using C++ virtual interfaces (pure abstract classes) oriented on Object-Oriented Programming paradigm
All interfaces has their own version which used to access them from the module interface factory via CreateInterfaceFn
New API will be used in each new OGS module (currently render and new game dlls oriented on this new API)  
It will be similar to Source engine API in some parts (like the IEngineSound interface)

## Interface set overview:

* **IMemory** - shared memory system interface that allows to allocate/free the memory from mempools
* **IFileSystem** - custom implementation of filesystem
* **IEngineSound** - shared sound system handler for both client/server (server impl will just send messages to clients to actually play/cache sound files) (**unsure**)
* **IClientGame** - main interface from client game module to init/shutdown and update it
* **IGame** - main game module interface
* **IGameClient** - represents a game client on a server (various client data could be accessed from here; also used to control the client during his play (not his world entity but his connection and data)  

**Example:**
```cpp
IGameClient *pGameClient = pGameServer->GetClientByName("player");

if(!pGameClient)
    return;

pGameClient->Kick("Please change your nickname!");