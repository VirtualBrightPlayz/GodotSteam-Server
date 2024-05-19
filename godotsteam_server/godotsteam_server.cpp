/////////////////////////////////////////////////
///// SILENCE STEAMWORKS WARNINGS
/////////////////////////////////////////////////
//
// Turn off MSVC-only warning about strcpy
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable:4996)
#pragma warning(disable:4828)
#endif


///// HEADER INCLUDES
/////////////////////////////////////////////////
//
// Include GodotSteam Server header
#include "godotsteam_server.h"

// Include some system headers
#include "string.h"
#include "fstream"
#include "vector"

using namespace godot;


///// STEAM OBJECT WITH CALLBACKS
/////////////////////////////////////////////////
//
SteamServer::SteamServer():
	// Game Server callbacks ////////////////////
	callbackServerConnectFailure(this, &SteamServer::server_connect_failure),
	callbackServerConnected(this, &SteamServer::server_connected),
	callbackServerDisconnected(this, &SteamServer::server_disconnected),
	callbackClientApproved(this, &SteamServer::client_approved),
	callbackClientDenied(this, &SteamServer::client_denied),
	callbackClientKicked(this, &SteamServer::client_kick),
	callbackPolicyResponse(this, &SteamServer::policy_response),
	callbackClientGroupStatus(this, &SteamServer::client_group_status),
	callbackAssociateClan(this, &SteamServer::associate_clan),
	callbackPlayerCompat(this, &SteamServer::player_compat),

	// Game Server Stat callbacks ///////////////
	callbackStatsStored(this, &SteamServer::stats_stored),
	callbackStatsUnloaded(this, &SteamServer::stats_unloaded),

	// HTTP callbacks ///////////////////////////
	callbackHTTPRequestCompleted(this, &SteamServer::http_request_completed),
	callbackHTTPRequestDataReceived(this, &SteamServer::http_request_data_received),
	callbackHTTPRequestHeadersReceived(this, &SteamServer::http_request_headers_received),

	// Inventory callbacks //////////////////////
	callbackInventoryDefinitionUpdate(this, &SteamServer::inventory_definition_update),
	callbackInventoryFullUpdate(this, &SteamServer::inventory_full_update),
	callbackInventoryResultReady(this, &SteamServer::inventory_result_ready),

	// Networking callbacks /////////////////////
	callbackP2PSessionConnectFail(this, &SteamServer::p2p_session_connect_fail),
	callbackP2PSessionRequest(this, &SteamServer::p2p_session_request),

	// Networking Messages callbacks ////////////
	callbackNetworkMessagesSessionRequest(this, &SteamServer::network_messages_session_request),
	callbackNetworkMessagesSessionFailed(this, &SteamServer::network_messages_session_failed),

	// Networking Sockets callbacks /////////////
	callbackNetworkConnectionStatusChanged(this, &SteamServer::network_connection_status_changed),
	callbackNetworkAuthenticationStatus(this, &SteamServer::network_authentication_status),
	callbackNetworkingFakeIPResult(this, &SteamServer::fake_ip_result),

	// Networking Utils callbacks ///////////////
	callbackRelayNetworkStatus(this, &SteamServer::relay_network_status),

	// Remote Storage callbacks /////////////////
	callbackLocalFileChanged(this, &SteamServer::local_file_changed),

	// UGC callbacks ////////////////////////////
	callbackItemDownloaded(this, &SteamServer::item_downloaded),
	callbackItemInstalled(this, &SteamServer::item_installed),
	callbackUserSubscribedItemsListChanged(this, &SteamServer::user_subscribed_items_list_changed)
{
	is_init_success = false;
}


///// INTERNAL FUNCTIONS
/////////////////////////////////////////////////
//
// Helper function to turn an array of options into an array of SteamNetworkingConfigValue_t structs
const SteamNetworkingConfigValue_t *SteamServer::convertOptionsArray(Array options) {
	// Get the number of option arrays in the array.
	int options_size = options.size();
	// Create the array for options.
	SteamNetworkingConfigValue_t *option_array = new SteamNetworkingConfigValue_t[options_size];
	// If there are options
	if (options_size > 0) {
		// Populate the options
		for (int i = 0; i < options_size; i++) {
			SteamNetworkingConfigValue_t this_option;
			Array sent_option = options[i];
			// Get the configuration value.
			// This is a convoluted way of doing it but can't seem to cast the value as an enum so here we are.
			ESteamNetworkingConfigValue this_value = ESteamNetworkingConfigValue((int)sent_option[0]);
			if ((int)sent_option[1] == 1) {
				this_option.SetInt32(this_value, sent_option[2]);
			}
			else if ((int)sent_option[1] == 2) {
				this_option.SetInt64(this_value, sent_option[2]);
			}
			else if ((int)sent_option[1] == 3) {
				this_option.SetFloat(this_value, sent_option[2]);
			}
			else if ((int)sent_option[1] == 4) {
				char *this_string = { 0 };
				String passed_string = sent_option[2];
				strcpy(this_string, passed_string.utf8().get_data());
				this_option.SetString(this_value, this_string);
			}
			else {
				Object *this_pointer;
				this_pointer = sent_option[2];
				this_option.SetPtr(this_value, this_pointer);
			}
			option_array[i] = this_option;
		}
	}
	return option_array;
}

// Creating a Steam ID for internal use
CSteamID SteamServer::createSteamID(uint64_t steam_id){
	CSteamID converted_steam_id;
	converted_steam_id.SetFromUint64(steam_id);
	return converted_steam_id;
}

// Convert a Steam ID to a Steam Identity
SteamNetworkingIdentity SteamServer::getIdentityFromSteamID(uint64_t steam_id) {
	SteamNetworkingIdentity remote_identity;
	remote_identity.SetSteamID64(steam_id);
	return remote_identity;
}

// Convert a string IP address to an integer
uint32 SteamServer::getIPFromString(String ip_string) {
	uint32 ip_address = 0;

	SteamNetworkingIPAddr this_address;
	this_address.Clear();
	
	if (this_address.ParseString(ip_string.utf8().get_data())) {
		ip_address = this_address.GetIPv4();
	}
	return ip_address;
}

// Convert a Steam IP Address object to an integer
uint32 SteamServer::getIPFromSteamIP(SteamNetworkingIPAddr this_address) {
	return this_address.GetIPv4();
}

// Get the Steam ID from an identity struct
uint64_t SteamServer::getSteamIDFromIdentity(SteamNetworkingIdentity this_identity) {
	uint64_t this_steam_id = this_identity.GetSteamID64();
	return this_steam_id;
}

// Convert an integer to a Steam IP Address
SteamNetworkingIPAddr SteamServer::getSteamIPFromInt(uint32 ip_integer) {
	SteamNetworkingIPAddr this_address;
	this_address.Clear();

	if (ip_integer > 0) {
		this_address.SetIPv4(ip_integer, 0);
	}
	return this_address;
}

// Convert an IP string to a Steam IP Address
SteamNetworkingIPAddr SteamServer::getSteamIPFromString(String ip_string) {
	SteamNetworkingIPAddr this_address;
	this_address.Clear();
	
	if (this_address.ParseString(ip_string.utf8().get_data())) {
		this_address.GetIPv4();
	}
	return this_address;
}

// Convert an integer IP address to a string
String SteamServer::getStringFromIP(uint32 ip_integer) {
	String ip_address = "";

	SteamNetworkingIPAddr this_address;
	this_address.Clear();

	if (ip_integer > 0) {
		this_address.SetIPv4(ip_integer, 0);
		char this_ip[16];
		this_address.ToString(this_ip, 16, false);
		ip_address = String(this_ip);
	}
	return ip_address;
}

// Convert a Steam IP Address to a string
String SteamServer::getStringFromSteamIP(SteamNetworkingIPAddr this_address) {
	char this_ip[16];
	this_address.ToString(this_ip, 16, false);
	return String(this_ip);
}


///// MAIN FUNCTIONS
/////////////////////////////////////////////////
//
// Convert a SteamID64 into a SteamID
uint32_t SteamServer::getSteamID32(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.GetAccountID();
}

// Gets the server's Steam ID.
uint64_t SteamServer::getServerSteamID(){
	return SteamGameServer_GetSteamID();
}

// Is this an anonymous account?
bool SteamServer::isAnonAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BAnonAccount();
}

// Is this an anonymous user account? Used to create an account or reset a password, but do not try to do this.
bool SteamServer::isAnonUserAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BAnonUserAccount();
}

// Is this a chat account ID?
bool SteamServer::isChatAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BChatAccount();
}

// Is this a clan account ID?
bool SteamServer::isClanAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BClanAccount();
}

// Is this a faked up Steam ID for a PSN friend account?
bool SteamServer::isConsoleUserAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BConsoleUserAccount();
}

// Is this an individual user account ID?
bool SteamServer::isIndividualAccount(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.BIndividualAccount();
}

// Is this a lobby account ID?
bool SteamServer::isLobby(uint64_t steam_id) {
	CSteamID this_steam_id = (uint64)steam_id;
	return this_steam_id.IsLobby();
}

// No official notes, but should be checking if the server is secured.
bool SteamServer::isServerSecure(){
	return SteamGameServer_BSecure();
}

// Initialize SteamGameServer client and interface objects, and set server properties which may not be changed.
// After calling this function, you should set any additional server parameters, and then logOnAnonymous() or logOn().
bool SteamServer::serverInit(String ip, uint16 game_port, uint16 query_port, int server_mode, String version_number){
	if(!SteamGameServer_Init(getIPFromString(ip), (uint16)game_port, (uint16)query_port, (EServerMode)server_mode, version_number.utf8().get_data())){
		return false;
	}

	return true;
}

// Initialize SteamGameServer client and interface objects, and set server properties which may not be changed.
// After calling this function, you should set any additional server parameters, and then logOnAnonymous() or logOn().
// On success STEAM_API_INIT_RESULT_OK is returned.  Otherwise, if error_message is non-NULL, it will receive a non-localized message that explains the reason for the failure
Dictionary SteamServer::serverInitEx(String ip, uint16 game_port, uint16 query_port, int server_mode, String version_number){
	char error_message[STEAM_MAX_ERROR_MESSAGE] = "Server initialized successfully";
	ESteamAPIInitResult initialize_result = k_ESteamAPIInitResult_FailedGeneric;
	initialize_result = SteamGameServer_InitEx(getIPFromString(ip), game_port, query_port, (EServerMode)server_mode, version_number.utf8().get_data(), &error_message);

	Dictionary server_initialize;
	server_initialize["status"] = initialize_result;
	server_initialize["verbal"] = error_message;

	return server_initialize;
}

// Frees all API-related memory associated with the calling thread. This memory is released automatically by RunCallbacks so single-threaded servers do not need to call this.
void SteamServer::serverReleaseCurrentThreadMemory(){
	SteamAPI_ReleaseCurrentThreadMemory();
}

// Shut down the server connection to Steam.
void SteamServer::serverShutdown(){
	SteamGameServer_Shutdown();
}


///// GAME SERVER FUNCTIONS
/////////////////////////////////////////////////
//
// NOTE: The following, if set, must be set before calling LogOn; they may not be changed after.
//
// Game product identifier; currently used by the master server for version checking purposes.
void SteamServer::setProduct(String product){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetProduct(product.utf8().get_data());
}

// Description of the game; required field and is displayed in the Steam server browser.
void SteamServer::setGameDescription(String description){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetGameDescription(description.utf8().get_data());
}

// If your game is a mod, pass the string that identifies it. Default is empty meaning the app is the original game.
void SteamServer::setModDir(String mod_directory){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetModDir(mod_directory.utf8().get_data());
}

// Is this a dedicated server? Default is false.
void SteamServer::setDedicatedServer(bool dedicated){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetDedicatedServer(dedicated);
}

// NOTE: The following are login functions.
//
// Begin process to login to a persistent game server account. You need to register for callbacks to determine the result of this operation.
void SteamServer::logOn(String token){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->LogOn(token.utf8().get_data());
}

// Login to a generic, anonymous account.
void SteamServer::logOnAnonymous(){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->LogOnAnonymous();
}

// Begin process of logging game server out of Steam.
void SteamServer::logOff(){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->LogOff();
}

// Status functions.
bool SteamServer::loggedOn(){
	if(SteamGameServer() == NULL){
		return false;
	}
	return SteamGameServer()->BLoggedOn();
}

bool SteamServer::secure(){
	if(SteamGameServer() == NULL){
		return false;
	}
	return SteamGameServer()->BSecure();
}

uint64_t SteamServer::getSteamID(){
	if(SteamGameServer() == NULL){
		return 0;
	}
	CSteamID serverID = SteamGameServer()->GetSteamID();
	return serverID.ConvertToUint64();
}

// Returns true if the master server has requested a restart. Only returns true once per request.
bool SteamServer::wasRestartRequested(){
	if(SteamGameServer() == NULL){
		return false;
	}
	return SteamGameServer()->WasRestartRequested();
}

// NOTE: These are server state functions and can be changed at any time.
//
// Max player count that will be reported to server browser and client queries.
void SteamServer::setMaxPlayerCount(int players_max){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetMaxPlayerCount(players_max);
}

// Number of bots. Default is zero.
void SteamServer::setBotPlayerCount(int bots){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetBotPlayerCount(bots);
}

// Set the naem of the server as it will appear in the server browser.
void SteamServer::setServerName(String name){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetServerName(name.utf8().get_data());
}

// Set name of map to report in server browser.
void SteamServer::setMapName(String map){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetMapName(map.utf8().get_data());
}

// Let people know if your server requires a password.
void SteamServer::setPasswordProtected(bool password_protected){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetPasswordProtected(password_protected);
}

// Spectator server. Default is zero, meaning it is now used.
void SteamServer::setSpectatorPort(uint16 port){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetSpectatorPort(port);
}

// Name of spectator server. Only used if spectator port is non-zero.
void SteamServer::setSpectatorServerName(String name){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetSpectatorServerName(name.utf8().get_data());
}

// Call this to clear the whole list of key/values that are sent in rule queries.
void SteamServer::clearAllKeyValues(){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->ClearAllKeyValues();
}

// Call this to add/update a key/value pair.
void SteamServer::setKeyValue(String key, String value){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetKeyValue(key.utf8().get_data(), value.utf8().get_data());
}

// Set a string defining game tags for this server; optional. Allows users to filter in matchmaking/server browser.
void SteamServer::setGameTags(String tags){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetGameTags(tags.utf8().get_data());
}

// Set a string defining game data for this server; optional. Allows users to filter in matchmaking/server browser.
void SteamServer::setGameData(String data){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetGameData(data.utf8().get_data());
}

// Region identifier; optional. Default is empty meaning 'world'.
void SteamServer::setRegion(String region){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetRegion(region.utf8().get_data());
}

// NOTE: These functions are player list management / authentication.
//
// Retrieve ticket to be sent to the entity who wishes to authenticate you (using BeginAuthSession API).
Dictionary SteamServer::getAuthSessionTicket(uint64_t remote_steam_id){
	// Create the dictionary to use
	Dictionary auth_ticket;
	if(SteamGameServer() != NULL){
		uint32_t id = 0;
		uint32_t ticket_size = 1024;
		PoolByteArray buffer;
		buffer.resize(ticket_size);
		// If no reference is passed, just use NULL
		// Not pretty but will work for now
		if(remote_steam_id == 0){
			SteamNetworkingIdentity auth_identity = getIdentityFromSteamID(remote_steam_id);
			id = SteamGameServer()->GetAuthSessionTicket(buffer.write().ptr(), ticket_size, &ticket_size, &auth_identity);
		}
		else{
			id = SteamGameServer()->GetAuthSessionTicket(buffer.write().ptr(), ticket_size, &ticket_size, NULL);
		}
		// Add this data to the dictionary
		auth_ticket["id"] = id;
		auth_ticket["buffer"] = buffer;
		auth_ticket["size"] = ticket_size;
	}
	return auth_ticket;
}

// Authenticate the ticket from the entity Steam ID to be sure it is valid and isn't reused.
uint32 SteamServer::beginAuthSession(PoolByteArray ticket, int ticket_size, uint64_t steam_id){
	if(SteamGameServer() == NULL){
		return -1;
	}
	CSteamID authSteamID = createSteamID(steam_id);
	return SteamGameServer()->BeginAuthSession(ticket.read().ptr(), ticket_size, authSteamID);
}

// Stop tracking started by beginAuthSession; called when no longer playing game with this entity;
void SteamServer::endAuthSession(uint64_t steam_id){
	if(SteamGameServer() != NULL){
		CSteamID authSteamID = createSteamID(steam_id);
		SteamGameServer()->EndAuthSession(authSteamID);
	}
}

// Cancel auth ticket from getAuthSessionTicket; called when no longer playing game with the entity you gave the ticket to.
void SteamServer::cancelAuthTicket(uint32_t auth_ticket){
	if(SteamGameServer() != NULL){
		SteamGameServer()->CancelAuthTicket(auth_ticket);
	}
}

// After receiving a user's authentication data, and passing it to sendUserConnectAndAuthenticate, use to determine if user owns DLC
int SteamServer::userHasLicenceForApp(uint64_t steam_id, uint32 app_id){
	if(SteamGameServer() == NULL){
		return 0;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServer()->UserHasLicenseForApp(userID, (AppId_t)app_id);
}

// Ask if user is in specified group; results returned by GSUserGroupStatus_t.
bool SteamServer::requestUserGroupStatus(uint64_t steam_id, int group_id){
	if(SteamGameServer() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	CSteamID clan_id = (uint64)group_id;
	return SteamGameServer()->RequestUserGroupStatus(userID, clan_id);
}

// NOTE: These are in GameSocketShare mode, where instead of ISteamGameServer creating sockets to talk to master server, it lets the game use its socket to forward messages back and forth.
//
// These are used when you've elected to multiplex the game server's UDP socket rather than having the master server updater use its own sockets.
Dictionary SteamServer::handleIncomingPacket(int packet, String ip, uint16 port){
	Dictionary result;
	if(SteamGameServer() == NULL){
		return result;
	}
	PoolByteArray data;
	data.resize(packet);
	if(SteamGameServer()->HandleIncomingPacket(data.write().ptr(), packet, getIPFromString(ip), (uint16)port)){
		result["data"] = data;
	}
	return result;
}

// AFTER calling HandleIncomingPacket for any packets that came in that frame, call this. This gets a packet that the master server updater needs to send out on UDP. Returns 0 if there are no more packets.
Dictionary SteamServer::getNextOutgoingPacket(){
	Dictionary packet;
	if(SteamGameServer() == NULL){
		return packet;
	}
	PoolByteArray out;
	int maxOut = 16 * 1024;
	uint32 address;
	uint16 port;
	// Retrieve the packet information
	int length = SteamGameServer()->GetNextOutgoingPacket(&out, maxOut, &address, &port);
	// Place packet information in dictionary and return it
	packet["length"] = length;
	packet["out"] = out;
	packet["address"] = address;
	packet["port"] = port;
	return packet;
}

// Gets the public IP of the server according to Steam.
Dictionary SteamServer::getPublicIP(){
	Dictionary public_ip;
	if(SteamGameServer() != NULL){
		SteamIPAddress_t this_public_ip = SteamGameServer()->GetPublicIP();
		// Populate the dictionary for returning
		public_ip["ipv4"] = this_public_ip.m_unIPv4;
		public_ip["ipv6"] = this_public_ip.m_rgubIPv6;
		public_ip["type"] = this_public_ip.m_eType;
	}
	return public_ip;
}

// NOTE: These are heartbeat/advertisement functions.
//
// Call this as often as you like to tell the master server updater whether or not you want it to be active (default: off).
void SteamServer::setAdvertiseServerActive(bool active){
	if(SteamGameServer() == NULL){
		return;
	}
	SteamGameServer()->SetAdvertiseServerActive(active);
}

// Associate this game server with this clan for the purposes of computing player compatibility.
void SteamServer::associateWithClan(uint64_t clan_id){
	if(SteamGameServer() == NULL){
		return;
	}
	CSteamID group_id = (uint64)clan_id;
	SteamGameServer()->AssociateWithClan(group_id);
}

// Ask if any of the current players dont want to play with this new player - or vice versa.
void SteamServer::computeNewPlayerCompatibility(uint64_t steam_id){
	if(SteamGameServer() == NULL){
		return;
	}
	CSteamID userID = (uint64)steam_id;
	SteamGameServer()->ComputeNewPlayerCompatibility(userID);
}


///// GAME SERVER STATS
/////////////////////////////////////////////////
//
// Resets the unlock status of an achievement for the specified user.
bool SteamServer::clearUserAchievement(uint64_t steam_id, String name){
	if(SteamGameServerStats() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServerStats()->ClearUserAchievement(userID, name.utf8().get_data());
}

// Gets the unlock status of the Achievement.
Dictionary SteamServer::getUserAchievement(uint64_t steam_id, String name){
	// Set dictionary to fill in
	Dictionary achievement;
	if(SteamGameServerStats() == NULL){
		return achievement;
	}
	CSteamID user_id = (uint64)steam_id;
	bool unlocked = false;
	bool result = SteamGameServerStats()->GetUserAchievement(user_id, name.utf8().get_data(), &unlocked);
	// Populate the dictionary
	achievement["result"] = result;
	achievement["user"] = steam_id;
	achievement["name"] = name;
	achievement["unlocked"] = unlocked;
	return achievement;
}

// Gets the current value of the a stat for the specified user.
uint32_t SteamServer::getUserStatInt(uint64_t steam_id, String name){
	if(SteamGameServerStats() != NULL){
		CSteamID userID = (uint64)steam_id;
		int32 value = 0;
		if(SteamGameServerStats()->GetUserStat(userID, name.utf8().get_data(), &value)){
			return value;
		}
	}
	return 0;
}

// Gets the current value of the a stat for the specified user.
float SteamServer::getUserStatFloat(uint64_t steam_id, String name){
	if(SteamGameServerStats() != NULL){
		CSteamID userID = (uint64)steam_id;
		float value = 0.0;
		if(SteamGameServerStats()->GetUserStat(userID, name.utf8().get_data(), &value)){
			return value;
		}
	}
	return 0.0;
}

// Asynchronously downloads stats and achievements for the specified user from the server.
void SteamServer::requestUserStats(uint64_t steam_id){
	if(SteamGameServerStats() != NULL){
		CSteamID userID = (uint64)steam_id;
		SteamAPICall_t api_call = SteamGameServerStats()->RequestUserStats(userID);
		callResultStatReceived.Set(api_call, this, &SteamServer::stats_received);
	}
}

// Unlocks an achievement for the specified user.
bool SteamServer::setUserAchievement(uint64_t steam_id, String name){
	if(SteamGameServerStats() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServerStats()->SetUserAchievement(userID, name.utf8().get_data());
}

// Sets / updates the value of a given stat for the specified user.
bool SteamServer::setUserStatInt(uint64_t steam_id, String name, int32 stat){
	if(SteamGameServerStats() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServerStats()->SetUserStat(userID, name.utf8().get_data(), stat);
}

// Sets / updates the value of a given stat for the specified user.
bool SteamServer::setUserStatFloat(uint64_t steam_id, String name, float stat){
	if(SteamGameServerStats() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServerStats()->SetUserStat(userID, name.utf8().get_data(), stat);
}

// Send the changed stats and achievements data to the server for permanent storage for the specified user.
void SteamServer::storeUserStats(uint64_t steam_id){
	if(SteamGameServerStats() != NULL){
		CSteamID userID = (uint64)steam_id;
		SteamGameServerStats()->StoreUserStats(userID);
	}
}

// Updates an AVGRATE stat with new values for the specified user.
bool SteamServer::updateUserAvgRateStat(uint64_t steam_id, String name, float this_session, double session_length){
	if(SteamGameServerStats() == NULL){
		return false;
	}
	CSteamID userID = (uint64)steam_id;
	return SteamGameServerStats()->UpdateUserAvgRateStat(userID, name.utf8().get_data(), this_session, session_length);
}


///// HTTP
/////////////////////////////////////////////////
//
// Creates a cookie container to store cookies during the lifetime of the process. This API is just for during process lifetime, after steam restarts no cookies are persisted and you have no way to access the cookie container across repeat executions of your process.
uint32_t SteamServer::createCookieContainer(bool allow_responses_to_modify){
	if(SteamHTTP() == NULL){
		return 0;
	}
	return SteamHTTP()->CreateCookieContainer(allow_responses_to_modify);
}

// Initializes a new HTTP request.
uint32_t SteamServer::createHTTPRequest(int request_method, String absolute_url){
	if(SteamHTTP() != NULL){
		return SteamHTTP()->CreateHTTPRequest((EHTTPMethod)request_method, absolute_url.utf8().get_data());
	}
	return HTTPREQUEST_INVALID_HANDLE;
}

// Defers a request which has already been sent by moving it at the back of the queue.
bool SteamServer::deferHTTPRequest(uint32 request_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->DeferHTTPRequest(request_handle);
}

// Gets progress on downloading the body for the request.
float SteamServer::getHTTPDownloadProgressPct(uint32 request_handle){
	float percent_one = 0.0;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPDownloadProgressPct(request_handle, &percent_one);
	}
	return percent_one;
}

// Check if the reason the request failed was because we timed it out (rather than some harder failure).
bool SteamServer::getHTTPRequestWasTimedOut(uint32 request_handle){
	bool was_timed_out = false;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPRequestWasTimedOut(request_handle, &was_timed_out);
	}
	return was_timed_out;
}

// Gets the body data from an HTTP response.
PoolByteArray SteamServer::getHTTPResponseBodyData(uint32 request_handle, uint32 buffer_size){
	PoolByteArray body_data;
	body_data.resize(buffer_size);
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPResponseBodyData(request_handle, body_data.write().ptr(), buffer_size);
	}
	return body_data;
}

// Gets the size of the body data from an HTTP response.
uint32 SteamServer::getHTTPResponseBodySize(uint32 request_handle){
	uint32 body_size = 0;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPResponseBodySize(request_handle, &body_size);
	}
	return body_size;
}

// Checks if a header is present in an HTTP response and returns its size.
uint32 SteamServer::getHTTPResponseHeaderSize(uint32 request_handle, String header_name){
	uint32 response_header_size = 0;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPResponseHeaderSize(request_handle, header_name.utf8().get_data(), &response_header_size);
	}
	return response_header_size;
}

// Gets a header value from an HTTP response.
uint8 SteamServer::getHTTPResponseHeaderValue(uint32 request_handle, String header_name, uint32 buffer_size){
	uint8 value_buffer = 0;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPResponseHeaderValue(request_handle, header_name.utf8().get_data(), &value_buffer, buffer_size);
	}
	return value_buffer;
}

// Gets the body data from a streaming HTTP response.
uint8 SteamServer::getHTTPStreamingResponseBodyData(uint32 request_handle, uint32 offset, uint32 buffer_size){
	uint8 body_data_buffer = 0;
	if(SteamHTTP() != NULL){
		SteamHTTP()->GetHTTPStreamingResponseBodyData(request_handle, offset, &body_data_buffer, buffer_size);
	}
	return body_data_buffer;
}

// Prioritizes a request which has already been sent by moving it at the front of the queue.
bool SteamServer::prioritizeHTTPRequest(uint32 request_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->PrioritizeHTTPRequest(request_handle);
}

// Releases a cookie container, freeing the memory allocated within Steam.
bool SteamServer::releaseCookieContainer(uint32 cookie_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->ReleaseCookieContainer(cookie_handle);
}

// Releases an HTTP request handle, freeing the memory allocated within Steam.
bool SteamServer::releaseHTTPRequest(uint32 request_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->ReleaseHTTPRequest(request_handle);
}

// Sends an HTTP request.
bool SteamServer::sendHTTPRequest(uint32 request_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	SteamAPICall_t call_handle;
	return SteamHTTP()->SendHTTPRequest(request_handle, &call_handle);
}

// Sends an HTTP request and streams the response back in chunks.
bool SteamServer::sendHTTPRequestAndStreamResponse(uint32 request_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	SteamAPICall_t call_handle;
	return SteamHTTP()->SendHTTPRequestAndStreamResponse(request_handle, &call_handle);
}

// Adds a cookie to the specified cookie container that will be used with future requests.
bool SteamServer::setHTTPCookie(uint32 cookie_handle, String host, String url, String cookie){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetCookie(cookie_handle, host.utf8().get_data(), url.utf8().get_data(), cookie.utf8().get_data());
}

// Set an absolute timeout in milliseconds for the HTTP request. This is the total time timeout which is different than the network activity timeout which is set with SetHTTPRequestNetworkActivityTimeout which can bump everytime we get more data.
bool SteamServer::setHTTPRequestAbsoluteTimeoutMS(uint32 request_handle, uint32 milliseconds){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestAbsoluteTimeoutMS(request_handle, milliseconds);
}

// Set a context value for the request, which will be returned in the HTTPRequestCompleted_t callback after sending the request. This is just so the caller can easily keep track of which callbacks go with which request data. Must be called before sending the request.
bool SteamServer::setHTTPRequestContextValue(uint32 request_handle, uint64_t context_value){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestContextValue(request_handle, context_value);
}

// Associates a cookie container to use for an HTTP request.
bool SteamServer::setHTTPRequestCookieContainer(uint32 request_handle, uint32 cookie_handle){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestCookieContainer(request_handle, cookie_handle);
}

// Set a GET or POST parameter value on the HTTP request. Must be called prior to sending the request.
bool SteamServer::setHTTPRequestGetOrPostParameter(uint32 request_handle, String name, String value){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestGetOrPostParameter(request_handle, name.utf8().get_data(), value.utf8().get_data());
}

// Set a request header value for the HTTP request. Must be called before sending the request.
bool SteamServer::setHTTPRequestHeaderValue(uint32 request_handle, String header_name, String header_value){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestHeaderValue(request_handle, header_name.utf8().get_data(), header_value.utf8().get_data());
}

// Set the timeout in seconds for the HTTP request.
bool SteamServer::setHTTPRequestNetworkActivityTimeout(uint32 request_handle, uint32 timeout_seconds){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestNetworkActivityTimeout(request_handle, timeout_seconds);
}

// Sets the body for an HTTP Post request.
uint8 SteamServer::setHTTPRequestRawPostBody(uint32 request_handle, String content_type, uint32 body_length){
	uint8 body = 0;
	if(SteamHTTP()){
		SteamHTTP()->SetHTTPRequestRawPostBody(request_handle, content_type.utf8().get_data(), &body, body_length);
	}
	return body;
}

// Sets that the HTTPS request should require verified SSL certificate via machines certificate trust store. This currently only works Windows and macOS.
bool SteamServer::setHTTPRequestRequiresVerifiedCertificate(uint32 request_handle, bool require_verified_certificate){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestRequiresVerifiedCertificate(request_handle, require_verified_certificate);
}

// Set additional user agent info for a request.
bool SteamServer::setHTTPRequestUserAgentInfo(uint32 request_handle, String user_agent_info){
	if(SteamHTTP() == NULL){
		return false;
	}
	return SteamHTTP()->SetHTTPRequestUserAgentInfo(request_handle, user_agent_info.utf8().get_data());
}


///// INVENTORY
/////
///// When dealing with any inventory handles, you should call CheckResultSteamID on the result handle when it completes to verify that a remote player is not pretending to have a different user's inventory.
///// Also, you must call DestroyResult on the provided inventory result when you are done with it.
/////////////////////////////////////////////////
//
// Grant a specific one-time promotional item to the current user.
int32 SteamServer::addPromoItem(uint32 item){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(SteamInventory()->AddPromoItem(&new_inventory_handle, item)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Grant a specific one-tsime promotional items to the current user.
int32 SteamServer::addPromoItems(PoolIntArray items){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		int count = items.size();
		SteamItemDef_t *new_items = new SteamItemDef_t[items.size()];
		for(int i = 0; i < count; i++){
			new_items[i] = items[i];
		}
		if(SteamInventory()->AddPromoItems(&new_inventory_handle, new_items, count)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
		delete[] new_items;
	}
	return new_inventory_handle;
}

// Checks whether an inventory result handle belongs to the specified Steam ID.
bool SteamServer::checkResultSteamID(uint64_t steam_id_expected, int32 this_inventory_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	CSteamID steam_id = (uint64)steam_id_expected;
	// If no inventory handle is passed, use internal one
	if(this_inventory_handle == 0){
		this_inventory_handle = inventory_handle;
	}
	return SteamInventory()->CheckResultSteamID((SteamInventoryResult_t)this_inventory_handle, steam_id);
}

// Consumes items from a user's inventory. If the quantity of the given item goes to zero, it is permanently removed.
int32 SteamServer::consumeItem(uint64_t item_consume, uint32 quantity){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
	 	if(SteamInventory()->ConsumeItem(&new_inventory_handle, (SteamItemInstanceID_t)item_consume, quantity)){
	 		// Update the internally stored handle
			inventory_handle = new_inventory_handle;
	 	}
	}
	return new_inventory_handle;
}

// Deserializes a result set and verifies the signature bytes.
int32 SteamServer::deserializeResult(PoolByteArray buffer){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(SteamInventory()->DeserializeResult(&new_inventory_handle, &buffer, buffer.size(), false)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Destroys a result handle and frees all associated memory.
void SteamServer::destroyResult(int this_inventory_handle){
	if(SteamInventory() != NULL){
		// If no inventory handle is passed, use internal one
		if(this_inventory_handle == 0){
			this_inventory_handle = inventory_handle;
		}	
		SteamInventory()->DestroyResult((SteamInventoryResult_t)this_inventory_handle);
	}
}

// Grant one item in exchange for a set of other items.
int32 SteamServer::exchangeItems(PoolIntArray output_items, PoolIntArray output_quantity, PoolIntArray input_items, PoolIntArray input_quantity){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		uint32_t* quantity_out = (uint32*) output_quantity.read().ptr();
		uint32_t* quantity_in = (uint32*) input_quantity.read().ptr();
		int array_size = input_items.size();
		SteamItemInstanceID_t *input_item_ids = new SteamItemInstanceID_t[array_size];
		for(int i = 0; i < array_size; i++){
			input_item_ids[i] = input_items[i];
		}
		const SteamItemInstanceID_t *these_item_ids = input_item_ids;
		if(SteamInventory()->ExchangeItems(&new_inventory_handle, output_items.read().ptr(), quantity_out, 1, these_item_ids, quantity_in, 1)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
		delete[] input_item_ids;
	}
	return new_inventory_handle;
}

// Grants specific items to the current user, for developers only.
int32 SteamServer::generateItems(PoolIntArray items, PoolIntArray quantity){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		uint32_t* this_quantity = (uint32*) quantity.read().ptr();
		if(SteamInventory()->GenerateItems(&new_inventory_handle, items.read().ptr(), this_quantity, items.size())){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Start retrieving all items in the current users inventory.
int32 SteamServer::getAllItems(){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(SteamInventory()->GetAllItems(&new_inventory_handle)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Gets a string property from the specified item definition.  Gets a property value for a specific item definition.
String SteamServer::getItemDefinitionProperty(uint32 definition, String name){
	if(SteamInventory() == NULL){
		return "";
	}
	uint32 buffer_size = STEAM_BUFFER_SIZE;
	char *buffer = new char[buffer_size];
	SteamInventory()->GetItemDefinitionProperty(definition, name.utf8().get_data(), buffer, &buffer_size);
	String property = buffer;
	delete[] buffer;
	return property;
}

// Gets the state of a subset of the current user's inventory.
int32 SteamServer::getItemsByID(PoolIntArray id_array){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		int array_size = id_array.size();
		SteamItemInstanceID_t *item_ids = new SteamItemInstanceID_t[array_size];
		for(int i = 0; i < array_size; i++){
			item_ids[i] = id_array[i];
		}
		const SteamItemInstanceID_t *these_item_ids = item_ids;
		if(SteamInventory()->GetItemsByID(&new_inventory_handle, these_item_ids, array_size)) {
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
		delete[] item_ids;
	}
	return new_inventory_handle;
}

// After a successful call to RequestPrices, you can call this method to get the pricing for a specific item definition.
Dictionary SteamServer::getItemPrice(uint32 definition) {
	Dictionary prices;
	if (SteamInventory() == NULL) {
		return prices;
	}
	uint64 price = 0;
	uint64 base_price = 0;
	SteamInventory()->GetItemPrice(definition, &price, &base_price);
	prices["price"] = (uint64_t)price;
	prices["base_price"] = (uint64_t)base_price;
	return prices;
}

// After a successful call to RequestPrices, you can call this method to get all the pricing for applicable item definitions. Use the result of GetNumItemsWithPrices as the the size of the arrays that you pass in.
Array SteamServer::getItemsWithPrices(){
	if(SteamInventory() == NULL){
		return Array();
	}
	uint32 valid_prices = SteamInventory()->GetNumItemsWithPrices();
	// Create the return array
	Array price_array;
	// Create a temporary array
	SteamItemDef_t *ids = new SteamItemDef_t[valid_prices];
	uint64 *prices = new uint64[valid_prices];
	uint64 *base_prices = new uint64[valid_prices];
	if(SteamInventory()->GetItemsWithPrices(ids, prices, base_prices, valid_prices)){
		for(uint32 i = 0; i < valid_prices; i++){
			Dictionary price_group;
			price_group["item"] = ids[i];
			price_group["price"] = (uint64_t)prices[i];
			price_group["base_prices"] = (uint64_t)base_prices[i];
			price_array.append(price_group);
		}
	}
	delete[] ids;
	delete[] prices;
	delete[] base_prices;
	return price_array;
}

// Gets the dynamic properties from an item in an inventory result set.
String SteamServer::getResultItemProperty(uint32 index, String name, int32 this_inventory_handle){
	if(SteamInventory() != NULL){
		// Set up variables to fill
		uint32 buffer_size = 256;
		char *value = new char[buffer_size];
		// If no inventory handle is passed, use internal one
		if(this_inventory_handle == 0){
			this_inventory_handle = inventory_handle;
		}
		SteamInventory()->GetResultItemProperty((SteamInventoryResult_t)this_inventory_handle, index, name.utf8().get_data(), (char*)value, &buffer_size);
		String property = value;
		delete[] value;
		return property;
	}
	return "";
}

// Get the items associated with an inventory result handle.
Array SteamServer::getResultItems(int32 this_inventory_handle){
	if(SteamInventory() == NULL){
		return Array();
	}
	// Set up return array
	Array items;
	uint32 size = 0;
	if(SteamInventory()->GetResultItems((SteamInventoryResult_t)this_inventory_handle, NULL, &size)){
		items.resize(size);
		SteamItemDetails_t *item_array = new SteamItemDetails_t[size];
		// If no inventory handle is passed, use internal one
		if(this_inventory_handle == 0){
			this_inventory_handle = inventory_handle;
		}
		if(SteamInventory()->GetResultItems((SteamInventoryResult_t)this_inventory_handle, item_array, &size)){
			for(uint32 i = 0; i < size; i++){
				Dictionary item_info;
				item_info["item_id"] = (uint64_t)item_array[i].m_itemId;
				item_info["item_definition"] = item_array[i].m_iDefinition;
				item_info["flags"] = item_array[i].m_unFlags;
				item_info["quantity"] = item_array[i].m_unQuantity;
				items.append(item_info);
			}
		}
		delete[] item_array;
	}
	return items;
}

// Find out the status of an asynchronous inventory result handle.
int SteamServer::getResultStatus(int32 this_inventory_handle){
	if(SteamInventory() == NULL){
		return k_EResultFail;
	}
	// If no inventory handle is passed, use internal one
	if(this_inventory_handle == 0){
		this_inventory_handle = inventory_handle;
	}
	return SteamInventory()->GetResultStatus((SteamInventoryResult_t)this_inventory_handle);
}

// Gets the server time at which the result was generated.
uint32 SteamServer::getResultTimestamp(int32 this_inventory_handle){
	if(SteamInventory() == NULL){
		return 0;
	}
	// If no inventory handle is passed, use internal one
	if(this_inventory_handle == 0){
		this_inventory_handle = inventory_handle;
	}
	return SteamInventory()->GetResultTimestamp((SteamInventoryResult_t)this_inventory_handle);
}

// Grant all potential one-time promotional items to the current user.
int32 SteamServer::grantPromoItems(){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(SteamInventory()->GrantPromoItems(&new_inventory_handle)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Triggers an asynchronous load and refresh of item definitions.
bool SteamServer::loadItemDefinitions(){
	if(SteamInventory() == NULL){
		return false;
	}
	return SteamInventory()->LoadItemDefinitions();
}

// Request the list of "eligible" promo items that can be manually granted to the given user.
void SteamServer::requestEligiblePromoItemDefinitionsIDs(uint64_t steam_id){
	if(SteamInventory() != NULL){
		CSteamID user_id = (uint64)steam_id;
		SteamAPICall_t api_call = SteamInventory()->RequestEligiblePromoItemDefinitionsIDs(user_id);
		callResultEligiblePromoItemDefIDs.Set(api_call, this, &SteamServer::inventory_eligible_promo_item);
	}
}

// Request prices for all item definitions that can be purchased in the user's local currency. A SteamInventoryRequestPricesResult_t call result will be returned with the user's local currency code. After that, you can call GetNumItemsWithPrices and GetItemsWithPrices to get prices for all the known item definitions, or GetItemPrice for a specific item definition.
void SteamServer::requestPrices(){
	if(SteamInventory() != NULL){
		SteamAPICall_t api_call = SteamInventory()->RequestPrices();
		callResultRequestPrices.Set(api_call, this, &SteamServer::inventory_request_prices_result);
	}
}

// Serialized result sets contain a short signature which can't be forged or replayed across different game sessions.
String SteamServer::serializeResult(int32 this_inventory_handle){
	String result_serialized;
	if(SteamInventory() != NULL){
		// If no inventory handle is passed, use internal one
		if(this_inventory_handle == 0){
			this_inventory_handle = inventory_handle;
		}
		// Set up return array
		uint32 buffer_size = STEAM_BUFFER_SIZE;
		char *buffer = new char[buffer_size];
		if(SteamInventory()->SerializeResult((SteamInventoryResult_t)this_inventory_handle, buffer, &buffer_size)){
			result_serialized = buffer;
		}
		delete[] buffer;
	}
	return result_serialized;
}

// Starts the purchase process for the user, given a "shopping cart" of item definitions that the user would like to buy. The user will be prompted in the Steam Overlay to complete the purchase in their local currency, funding their Steam Wallet if necessary, etc.
void SteamServer::startPurchase(PoolIntArray items, PoolIntArray quantity){
	if(SteamInventory() != NULL){
		uint32_t* these_quantities = (uint32*) quantity.read().ptr();
		SteamAPICall_t api_call = SteamInventory()->StartPurchase(items.read().ptr(), these_quantities, items.size());
		callResultStartPurchase.Set(api_call, this, &SteamServer::inventory_start_purchase_result);
	}
}

// Transfer items between stacks within a user's inventory.
int32 SteamServer::transferItemQuantity(uint64_t item_id, uint32 quantity, uint64_t item_destination, bool split){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(split){
			if(SteamInventory()->TransferItemQuantity(&new_inventory_handle, (SteamItemInstanceID_t)item_id, quantity, k_SteamItemInstanceIDInvalid)){
				// Update the internally stored handle
				inventory_handle = new_inventory_handle;
			}
		}
		else{
			if(SteamInventory()->TransferItemQuantity(&new_inventory_handle, (SteamItemInstanceID_t)item_id, quantity, (SteamItemInstanceID_t)item_destination)){
				// Update the internally stored handle
				inventory_handle = new_inventory_handle;
			}
		}
	}
	return new_inventory_handle;
}

// Trigger an item drop if the user has played a long enough period of time.
int32 SteamServer::triggerItemDrop(uint32 definition){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		if(SteamInventory()->TriggerItemDrop(&new_inventory_handle, (SteamItemDef_t)definition)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Starts a transaction request to update dynamic properties on items for the current user. This call is rate-limited by user, so property modifications should be batched as much as possible (e.g. at the end of a map or game session). After calling SetProperty or RemoveProperty for all the items that you want to modify, you will need to call SubmitUpdateProperties to send the request to the Steam servers. A SteamInventoryResultReady_t callback will be fired with the results of the operation.
void SteamServer::startUpdateProperties(){
	if(SteamInventory() != NULL){
		inventory_update_handle = SteamInventory()->StartUpdateProperties();
	}
}

// Submits the transaction request to modify dynamic properties on items for the current user. See StartUpdateProperties.
int32 SteamServer::submitUpdateProperties(uint64_t this_inventory_update_handle){
	int32 new_inventory_handle = 0;
	if(SteamInventory() != NULL){
		// If no inventory update handle is passed, use internal one
		if(this_inventory_update_handle == 0){
			this_inventory_update_handle = inventory_update_handle;
		}
		if(SteamInventory()->SubmitUpdateProperties((SteamInventoryUpdateHandle_t)this_inventory_update_handle, &new_inventory_handle)){
			// Update the internally stored handle
			inventory_handle = new_inventory_handle;
		}
	}
	return new_inventory_handle;
}

// Removes a dynamic property for the given item.
bool SteamServer::removeProperty(uint64_t item_id, String name, uint64_t this_inventory_update_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	// If no inventory update handle is passed, use internal one
	if(this_inventory_update_handle == 0){
		this_inventory_update_handle = inventory_update_handle;
	}
	return SteamInventory()->RemoveProperty((SteamInventoryUpdateHandle_t)this_inventory_update_handle, (SteamItemInstanceID_t)item_id, name.utf8().get_data());
}

// Sets a dynamic property for the given item. Supported value types are strings.
bool SteamServer::setPropertyString(uint64_t item_id, String name, String value, uint64_t this_inventory_update_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	// If no inventory update handle is passed, use internal one
	if(this_inventory_update_handle == 0){
		this_inventory_update_handle = inventory_update_handle;
	}
	return SteamInventory()->SetProperty((SteamInventoryUpdateHandle_t)this_inventory_update_handle, (SteamItemInstanceID_t)item_id, name.utf8().get_data(), value.utf8().get_data());
}

// Sets a dynamic property for the given item. Supported value types are boolean.
bool SteamServer::setPropertyBool(uint64_t item_id, String name, bool value, uint64_t this_inventory_update_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	// If no inventory update handle is passed, use internal one
	if(this_inventory_update_handle == 0){
		this_inventory_update_handle = inventory_update_handle;
	}
	return SteamInventory()->SetProperty((SteamInventoryUpdateHandle_t)this_inventory_update_handle, (SteamItemInstanceID_t)item_id, name.utf8().get_data(), value);
}

// Sets a dynamic property for the given item. Supported value types are 64 bit integers.
bool SteamServer::setPropertyInt(uint64_t item_id, String name, uint64_t value, uint64_t this_inventory_update_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	// If no inventory update handle is passed, use internal one
	if(this_inventory_update_handle == 0){
		this_inventory_update_handle = inventory_update_handle;
	}
	return SteamInventory()->SetProperty((SteamInventoryUpdateHandle_t)this_inventory_update_handle, (SteamItemInstanceID_t)item_id, name.utf8().get_data(), (int64)value);
}

// Sets a dynamic property for the given item. Supported value types are 32 bit floats.
bool SteamServer::setPropertyFloat(uint64_t item_id, String name, float value, uint64_t this_inventory_update_handle){
	if(SteamInventory() == NULL){
		return false;
	}
	// If no inventory update handle is passed, use internal one
	if(this_inventory_update_handle == 0){
		this_inventory_update_handle = inventory_update_handle;
	}
	return SteamInventory()->SetProperty((SteamInventoryUpdateHandle_t)this_inventory_update_handle, (SteamItemInstanceID_t)item_id, name.utf8().get_data(), value);
}


///// NETWORKING
/////////////////////////////////////////////////
//
// This allows the game to specify accept an incoming packet.
bool SteamServer::acceptP2PSessionWithUser(uint64_t remote_steam_id) {
	if (SteamNetworking() == NULL) {
		return false;
	}
	CSteamID steam_id = createSteamID(remote_steam_id);
	return SteamNetworking()->AcceptP2PSessionWithUser(steam_id);
}

// Allow or disallow P2P connections to fall back to being relayed through the Steam servers if a direct connection or NAT-traversal cannot be established.
bool SteamServer::allowP2PPacketRelay(bool allow) {
	if (SteamNetworking() == NULL) {
		return false;
	}
	return SteamNetworking()->AllowP2PPacketRelay(allow);
}

// Closes a P2P channel when you're done talking to a user on the specific channel.
bool SteamServer::closeP2PChannelWithUser(uint64_t remote_steam_id, int channel) {
	if (SteamNetworking() == NULL) {
		return false;
	}
	CSteamID steam_id = createSteamID(remote_steam_id);
	return SteamNetworking()->CloseP2PChannelWithUser(steam_id, channel);
}

// This should be called when you're done communicating with a user, as this will free up all of the resources allocated for the connection under-the-hood.
bool SteamServer::closeP2PSessionWithUser(uint64_t remote_steam_id) {
	if (SteamNetworking() == NULL) {
		return false;
	}
	CSteamID steam_id = createSteamID(remote_steam_id);
	return SteamNetworking()->CloseP2PSessionWithUser(steam_id);
}

// Fills out a P2PSessionState_t structure with details about the connection like whether or not there is an active connection.
Dictionary SteamServer::getP2PSessionState(uint64_t remote_steam_id) {
	Dictionary result;
	if (SteamNetworking() == NULL) {
		return result;
	}
	CSteamID steam_id = createSteamID(remote_steam_id);
	P2PSessionState_t p2pSessionState;
	bool success = SteamNetworking()->GetP2PSessionState(steam_id, &p2pSessionState);
	if (!success) {
		return result;
	}
	result["connection_active"] = p2pSessionState.m_bConnectionActive; // true if we've got an active open connection
	result["connecting"] = p2pSessionState.m_bConnecting; // true if we're currently trying to establish a connection
	result["session_error"] = p2pSessionState.m_eP2PSessionError; // last error recorded (see enum in isteamnetworking.h)
	result["using_relay"] = p2pSessionState.m_bUsingRelay; // true if it's going through a relay server (TURN)
	result["bytes_queued_for_send"] = p2pSessionState.m_nBytesQueuedForSend;
	result["packets_queued_for_send"] = p2pSessionState.m_nPacketsQueuedForSend;
	result["remote_ip"] = p2pSessionState.m_nRemoteIP; // potential IP:Port of remote host. Could be TURN server.
	result["remote_port"] = p2pSessionState.m_nRemotePort; // Only exists for compatibility with older authentication api's
	return result;
}

// Calls IsP2PPacketAvailable() under the hood, returns the size of the available packet or zero if there is no such packet.
uint32_t SteamServer::getAvailableP2PPacketSize(int channel){
	if (SteamNetworking() == NULL) {
		return 0;
	}
	uint32_t messageSize = 0;
	return (SteamNetworking()->IsP2PPacketAvailable(&messageSize, channel)) ? messageSize : 0;
}

// Reads in a packet that has been sent from another user via SendP2PPacket.
Dictionary SteamServer::readP2PPacket(uint32_t packet, int channel){
	Dictionary result;
	if (SteamNetworking() == NULL) {
		return result;
	}
	PoolByteArray data;
	data.resize(packet);
	CSteamID steam_id;
	uint32_t bytesRead = 0;
	if (SteamNetworking()->ReadP2PPacket(data.write().ptr(), packet, &bytesRead, &steam_id, channel)){
		data.resize(bytesRead);
		uint64_t remote_steam_id = steam_id.ConvertToUint64();
		result["data"] = data;
		result["remote_steam_id"] = remote_steam_id;
	}
	else {
		data.resize(0);
	}
	return result;
}

// Sends a P2P packet to the specified user.
bool SteamServer::sendP2PPacket(uint64_t remote_steam_id, PoolByteArray data, int send_type, int channel){
	if (SteamNetworking() == NULL) {
		return false;
	}
	CSteamID steam_id = createSteamID(remote_steam_id);
	return SteamNetworking()->SendP2PPacket(steam_id, data.read().ptr(), data.size(), EP2PSend(send_type), channel);
}


///// NETWORKING MESSAGES
/////////////////////////////////////////////////
//
// AcceptSessionWithUser() should only be called in response to a SteamP2PSessionRequest_t callback SteamP2PSessionRequest_t will be posted if another user tries to send you a message, and you haven't tried to talk to them.
bool SteamServer::acceptSessionWithUser(uint64_t remote_steam_id){
	if(SteamNetworkingMessages() == NULL){
		return false;
	}
	return SteamNetworkingMessages()->AcceptSessionWithUser(getIdentityFromSteamID(remote_steam_id));
}

// Call this  when you're done talking to a user on a specific channel. Once all open channels to a user have been closed, the open session to the user will be closed, and any new data from this user will trigger a SteamP2PSessionRequest_t callback.
bool SteamServer::closeChannelWithUser(uint64_t remote_steam_id, int channel){
	if(SteamNetworkingMessages() == NULL){
		return false;
	}
	return SteamNetworkingMessages()->CloseChannelWithUser(getIdentityFromSteamID(remote_steam_id), channel);
}

// Call this when you're done talking to a user to immediately free up resources under-the-hood.
bool SteamServer::closeSessionWithUser(uint64_t remote_steam_id){
	if(SteamNetworkingMessages() == NULL){
		return false;
	}
	return SteamNetworkingMessages()->CloseSessionWithUser(getIdentityFromSteamID(remote_steam_id));
}

// Returns information about the latest state of a connection, if any, with the given peer.
Dictionary SteamServer::getSessionConnectionInfo(uint64_t remote_steam_id, bool get_connection, bool get_status){
	Dictionary connection_info;
	if(SteamNetworkingMessages() != NULL){
		SteamNetConnectionInfo_t this_info;
		SteamNetConnectionRealTimeStatus_t this_status;
		int connection_state = SteamNetworkingMessages()->GetSessionConnectionInfo(getIdentityFromSteamID(remote_steam_id), &this_info, &this_status);
		// Parse the data to a dictionary
		connection_info["connection_state"] = connection_state;
		// If getting the connection information
		if(get_connection){
			connection_info["identity"] = getSteamIDFromIdentity(this_info.m_identityRemote);
			connection_info["user_data"] = (uint64_t)this_info.m_nUserData;
			connection_info["listen_socket"] = this_info.m_hListenSocket;
			connection_info["remote_address"] = getStringFromSteamIP(this_info.m_addrRemote);
			connection_info["remote_pop"] = this_info.m_idPOPRemote;
			connection_info["pop_relay"] = this_info.m_idPOPRelay;
			connection_info["connection_state"] = this_info.m_eState;
			connection_info["end_reason"] = this_info.m_eEndReason;
			connection_info["end_debug"] = this_info.m_szEndDebug;
			connection_info["debug_description"] = this_info.m_szConnectionDescription;
		}
		// If getting the quick status
		if(get_status){
			connection_info["state"] = this_status.m_eState;
			connection_info["ping"] = this_status.m_nPing;
			connection_info["local_quality"] = this_status.m_flConnectionQualityLocal;
			connection_info["remote_quality"] = this_status.m_flConnectionQualityRemote;
			connection_info["packets_out_per_second"] = this_status.m_flOutPacketsPerSec;
			connection_info["bytes_out_per_second"] = this_status.m_flOutBytesPerSec;
			connection_info["packets_in_per_second"] = this_status.m_flInPacketsPerSec;
			connection_info["bytes_in_per_second"] = this_status.m_flInBytesPerSec;
			connection_info["send_rate"] = this_status.m_nSendRateBytesPerSecond;
			connection_info["pending_unreliable"] = this_status.m_cbPendingUnreliable;
			connection_info["pending_reliable"] = this_status.m_cbPendingReliable;
			connection_info["sent_unacknowledged_reliable"] = this_status.m_cbSentUnackedReliable;
			connection_info["queue_time"] = (uint64_t)this_status.m_usecQueueTime;
		}
	}
	return connection_info;
}

// Reads the next message that has been sent from another user via SendMessageToUser() on the given channel. Returns number of messages returned into your list.  (0 if no message are available on that channel.)
Array SteamServer::receiveMessagesOnChannel(int channel, int max_messages){
	Array messages;
	if(SteamNetworkingMessages() != NULL){
		// Allocate the space for the messages
		SteamNetworkingMessage_t** channel_messages = new SteamNetworkingMessage_t*[max_messages];
		// Get the messages
		int available_messages = SteamNetworkingMessages()->ReceiveMessagesOnChannel(channel, channel_messages, max_messages);
		// Loop through and create the messages as dictionaries then add to the messages array
		for(int i = 0; i < available_messages; i++){
			// Set up the mesage dictionary
			Dictionary message;
			// Get the data / message
			int message_size = channel_messages[i]->m_cbSize;
			PoolByteArray data;
			data.resize(message_size);
			uint8_t* source_data = (uint8_t*)channel_messages[i]->m_pData;
			uint8_t* output_data = data.write().ptr();
			for(int j = 0; j < message_size; j++){
				output_data[j] = source_data[j];
			}
			message["payload"] = data;
			message["size"] = message_size;
			message["connection"] = channel_messages[i]->m_conn;
			message["identity"] = getSteamIDFromIdentity(channel_messages[i]->m_identityPeer);
			message["receiver_user_data"] = (uint64_t)channel_messages[i]->m_nConnUserData;	// Not used when sending messages
			message["time_received"] = (uint64_t)channel_messages[i]->m_usecTimeReceived;
			message["message_number"] = (uint64_t)channel_messages[i]->m_nMessageNumber;
			message["channel"] = channel_messages[i]->m_nChannel;
			message["flags"] = channel_messages[i]->m_nFlags;
			message["sender_user_data"] = (uint64_t)channel_messages[i]->m_nUserData;	// Not used when receiving messages
			messages.append(message);
			// Release the message
			channel_messages[i]->Release();
		}
		delete [] channel_messages;
	}
	return messages;
}

// Sends a message to the specified host. If we don't already have a session with that user, a session is implicitly created. There might be some handshaking that needs to happen before we can actually begin sending message data.
int SteamServer::sendMessageToUser(uint64_t remote_steam_id, const PoolByteArray data, int flags, int channel){
	if(SteamNetworkingMessages() == NULL){
		return 0;
	}
	return SteamNetworkingMessages()->SendMessageToUser(getIdentityFromSteamID(remote_steam_id), data.read().ptr(), data.size(), flags, channel);
}


///// NETWORKING SOCKETS
/////////////////////////////////////////////////
//
// Creates a "server" socket that listens for clients to connect to by calling ConnectByIPAddress, over ordinary UDP (IPv4 or IPv6)
uint32 SteamServer::createListenSocketIP(String ip_address, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	uint32 listen_socket = SteamNetworkingSockets()->CreateListenSocketIP(getSteamIPFromString(ip_address), options.size(), convertOptionsArray(options));
	return listen_socket;
}

// Like CreateListenSocketIP, but clients will connect using ConnectP2P. The connection will be relayed through the Valve network.
uint32 SteamServer::createListenSocketP2P(int virtual_port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	uint32 listen_socket = SteamNetworkingSockets()->CreateListenSocketP2P(virtual_port, options.size(), convertOptionsArray(options));
	return listen_socket;
}

// Begin connecting to a server that is identified using a platform-specific identifier. This uses the default rendezvous service, which depends on the platform and library configuration. (E.g. on Steam, it goes through the steam backend.) The traffic is relayed over the Steam Datagram Relay network.
uint32 SteamServer::connectP2P(uint64_t remote_steam_id, int virtual_port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->ConnectP2P(getIdentityFromSteamID(remote_steam_id), virtual_port, sizeof(options), convertOptionsArray(options));
}

// Begin connecting to a server listen socket that is identified using an [ip-address]:[port], i.e. 127.0.0.1:27015. Used with createListenSocketIP
uint32 SteamServer::connectByIPAddress(String ip_address_with_port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->ConnectByIPAddress(getSteamIPFromString(ip_address_with_port), options.size(), convertOptionsArray(options));
}

// Client call to connect to a server hosted in a Valve data center, on the specified virtual port. You must have placed a ticket for this server into the cache, or else this connect attempt will fail!
uint32 SteamServer::connectToHostedDedicatedServer(uint64_t remote_steam_id, int virtual_port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	uint32 listen_socket = SteamNetworkingSockets()->ConnectToHostedDedicatedServer(getIdentityFromSteamID(remote_steam_id), virtual_port, options.size(), convertOptionsArray(options));
	return listen_socket;
}

// Accept an incoming connection that has been received on a listen socket.
int SteamServer::acceptConnection(uint32 connection_handle){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->AcceptConnection((HSteamNetConnection)connection_handle);
}

// Disconnects from the remote host and invalidates the connection handle. Any unread data on the connection is discarded.
bool SteamServer::closeConnection(uint32 peer, int reason, String debug_message, bool linger){
	if(SteamNetworkingSockets() == NULL){
		return false;
	}
	return SteamNetworkingSockets()->CloseConnection((HSteamNetConnection)peer, reason, debug_message.utf8().get_data(), linger);
}

// Destroy a listen socket. All the connections that were accepted on the listen socket are closed ungracefully.
bool SteamServer::closeListenSocket(uint32 socket){
	if(SteamNetworkingSockets() == NULL){
		return false;
	}
	return SteamNetworkingSockets()->CloseListenSocket((HSteamListenSocket)socket);
}

// Create a pair of connections that are talking to each other, e.g. a loopback connection. This is very useful for testing, or so that your client/server code can work the same even when you are running a local "server".
Dictionary SteamServer::createSocketPair(bool loopback, uint64_t remote_steam_id1, uint64_t remote_steam_id2){
	// Create a dictionary to populate
	Dictionary connection_pair;
	if(SteamNetworkingSockets() != NULL){
		// Get connections
		uint32 connection1 = 0;
		uint32 connection2 = 0;
		SteamNetworkingIdentity remote_identity1 = getIdentityFromSteamID(remote_steam_id1);
		SteamNetworkingIdentity remote_identity2 = getIdentityFromSteamID(remote_steam_id2);
		bool success = SteamNetworkingSockets()->CreateSocketPair(&connection1, &connection2, loopback, &remote_identity1, &remote_identity2);
		// Populate the dictionary
		connection_pair["success"] = success;
		connection_pair["connection1"] = connection1;
		connection_pair["connection2"] = connection2;
	}
	return connection_pair;
}

// Send a message to the remote host on the specified connection.
Dictionary SteamServer::sendMessageToConnection(uint32 connection_handle, const PoolByteArray data, int flags){
	Dictionary message_response;
	if(SteamNetworkingSockets() != NULL){
		int64 number;
		int result = SteamNetworkingSockets()->SendMessageToConnection((HSteamNetConnection)connection_handle, data.read().ptr(), data.size(), flags, &number);
		// Populate the dictionary
		message_response["result"] = result;
		message_response["message_number"] = (uint64_t)number;
	}
	return message_response;
}

// Send one or more messages without copying the message payload. This is the most efficient way to send messages. To use this function, you must first allocate a message object using ISteamNetworkingUtils::AllocateMessage. (Do not declare one on the stack or allocate your own.)
void SteamServer::sendMessages(int messages, const PoolByteArray data, uint32 connection_handle, int flags){
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingMessage_t *networkMessage;
		networkMessage = SteamNetworkingUtils()->AllocateMessage(0);
		networkMessage->m_pData = (void *)data.read().ptr();
		networkMessage->m_cbSize = data.size();
		networkMessage->m_conn = (HSteamNetConnection)connection_handle;
		networkMessage->m_nFlags = flags;
		int64 result;
		SteamNetworkingSockets()->SendMessages(messages, &networkMessage, &result);
		// Release the message
		networkMessage->Release();
	}
}

// Flush any messages waiting on the Nagle timer and send them at the next transmission opportunity (often that means right now).
int SteamServer::flushMessagesOnConnection(uint32 connection_handle){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->FlushMessagesOnConnection((HSteamNetConnection)connection_handle);
}

// Fetch the next available message(s) from the connection, if any. Returns the number of messages returned into your array, up to nMaxMessages. If the connection handle is invalid, -1 is returned. If no data is available, 0, is returned.
Array SteamServer::receiveMessagesOnConnection(uint32 connection_handle, int max_messages){
	Array messages;
	if(SteamNetworkingSockets() != NULL){
		// Allocate the space for the messages
		SteamNetworkingMessage_t** connection_messages = new SteamNetworkingMessage_t*[max_messages];
		// Get the messages
		int available_messages = SteamNetworkingSockets()->ReceiveMessagesOnConnection((HSteamNetConnection)connection_handle, connection_messages, max_messages);
		// Loop through and create the messages as dictionaries then add to the messages array
		for(int i = 0; i < available_messages; i++){
			// Create the message dictionary to send back
			Dictionary message;
			// Get the message data
			int message_size = connection_messages[i]->m_cbSize;
			PoolByteArray data;
			data.resize(message_size);
			uint8_t* source_data = (uint8_t*)connection_messages[i]->m_pData;
			uint8_t* output_data = data.write().ptr();
			for(int j = 0; j < message_size; j++){
				output_data[j] = source_data[j];
			}
			message["payload"] = data;
			message["size"] = message_size;
			message["connection"] = connection_messages[i]->m_conn;
			message["identity"] = getSteamIDFromIdentity(connection_messages[i]->m_identityPeer);
			message["receiver_user_data"] = (uint64_t)connection_messages[i]->m_nConnUserData; // Not used when sending messages
			message["time_received"] = (uint64_t)connection_messages[i]->m_usecTimeReceived;
			message["message_number"] = (uint64_t)connection_messages[i]->m_nMessageNumber;
			message["channel"] = connection_messages[i]->m_nChannel;
			message["flags"] = connection_messages[i]->m_nFlags;
			message["sender_user_data"] = (uint64_t)connection_messages[i]->m_nUserData; // Not used when receiving messages
			messages.append(message);
			// Release the message
			connection_messages[i]->Release();
		}
		delete [] connection_messages;
	}
	return messages;
}

// Create a new poll group.
uint32 SteamServer::createPollGroup(){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->CreatePollGroup();
}

// Destroy a poll group created with CreatePollGroup.
bool SteamServer::destroyPollGroup(uint32 poll_group){
	if(SteamNetworkingSockets() == NULL){
		return false;
	}
	return SteamNetworkingSockets()->DestroyPollGroup((HSteamNetPollGroup)poll_group);
}

// Assign a connection to a poll group. Note that a connection may only belong to a single poll group. Adding a connection to a poll group implicitly removes it from any other poll group it is in.
bool SteamServer::setConnectionPollGroup(uint32 connection_handle, uint32 poll_group){
	if(SteamNetworkingSockets() == NULL){
		return false;
	}
	return SteamNetworkingSockets()->SetConnectionPollGroup((HSteamNetConnection)connection_handle, (HSteamNetPollGroup)poll_group);
}

// Same as ReceiveMessagesOnConnection, but will return the next messages available on any connection in the poll group. Examine SteamNetworkingMessage_t::m_conn to know which connection. (SteamNetworkingMessage_t::m_nConnUserData might also be useful.)
Array SteamServer::receiveMessagesOnPollGroup(uint32 poll_group, int max_messages){
	Array messages;
	if(SteamNetworkingSockets() != NULL){
		// Allocate the space for the messages
		SteamNetworkingMessage_t** poll_messages = new SteamNetworkingMessage_t*[max_messages];
		// Get the messages
		int available_messages = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup((HSteamNetPollGroup)poll_group, poll_messages, max_messages);
		// Loop through and create the messages as dictionaries then add to the messages array
		for(int i = 0; i < available_messages; i++){
			// Create the message dictionary to send back
			Dictionary message;
			// Get the message data
			int message_size = poll_messages[i]->m_cbSize;
			PoolByteArray data;
			data.resize(message_size);
			uint8_t* source_data = (uint8_t*)poll_messages[i]->m_pData;
			uint8_t* output_data = data.write().ptr();
			for(int j = 0; j < message_size; j++){
				output_data[j] = source_data[j];
			}
			message["payload"] = data;
			message["size"] = message_size;
			message["connection"] = poll_messages[i]->m_conn;
			message["identity"] = getSteamIDFromIdentity(poll_messages[i]->m_identityPeer);
			message["receiver_user_data"] = (uint64_t)poll_messages[i]->m_nConnUserData;
			message["time_received"] = (uint64_t)poll_messages[i]->m_usecTimeReceived;
			message["message_number"] = (uint64_t)poll_messages[i]->m_nMessageNumber;
			message["channel"] = poll_messages[i]->m_nChannel;
			message["flags"] = poll_messages[i]->m_nFlags;
			message["sender_user_data"] = (uint64_t)poll_messages[i]->m_nUserData;
			messages.append(message);
			// Release the message
			poll_messages[i]->Release();
		}
		delete [] poll_messages;
	}
	return messages;
}

// Returns basic information about the high-level state of the connection. Returns false if the connection handle is invalid.
Dictionary SteamServer::getConnectionInfo(uint32 connection_handle){
	Dictionary connection_info;
	if(SteamNetworkingSockets() != NULL){
		SteamNetConnectionInfo_t info;
		if(SteamNetworkingSockets()->GetConnectionInfo((HSteamNetConnection)connection_handle, &info)){
			connection_info["identity"] = getSteamIDFromIdentity(info.m_identityRemote);
			connection_info["user_data"] = (uint64_t)info.m_nUserData;
			connection_info["listen_socket"] = info.m_hListenSocket;
			connection_info["remote_address"] = getStringFromSteamIP(info.m_addrRemote);
			connection_info["remote_pop"] = info.m_idPOPRemote;
			connection_info["pop_relay"] = info.m_idPOPRelay;
			connection_info["connection_state"] = info.m_eState;
			connection_info["end_reason"] = info.m_eEndReason;
			connection_info["end_debug"] = info.m_szEndDebug;
			connection_info["debug_description"] = info.m_szConnectionDescription;
		}
	}
	return connection_info;
}

// Returns very detailed connection stats in diagnostic text format. Useful for dumping to a log, etc. The format of this information is subject to change.
Dictionary SteamServer::getDetailedConnectionStatus(uint32 connection){
	Dictionary connection_status;
	if (SteamNetworkingSockets() != NULL) {
		char buffer[STEAM_LARGE_BUFFER_SIZE];
		int success = SteamNetworkingSockets()->GetDetailedConnectionStatus((HSteamNetConnection)connection, buffer, STEAM_LARGE_BUFFER_SIZE);
		// Add data to dictionary
		connection_status["success"] = success;
		connection_status["status"] = buffer;
	}
	// Send the data back to the user
	return connection_status;
}

// Fetch connection user data. Returns -1 if handle is invalid or if you haven't set any userdata on the connection.
uint64_t SteamServer::getConnectionUserData(uint32 peer){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->GetConnectionUserData((HSteamNetConnection)peer);
}

// Set a name for the connection, used mostly for debugging
void SteamServer::setConnectionName(uint32 peer, String name){
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingSockets()->SetConnectionName((HSteamNetConnection)peer, name.utf8().get_data());
	}
}

// Fetch connection name into your buffer, which is at least nMaxLen bytes. Returns false if handle is invalid.
String SteamServer::getConnectionName(uint32 peer){
	// Set empty string variable for use
	String connection_name = "";
	if(SteamNetworkingSockets() != NULL){
		char name[STEAM_BUFFER_SIZE];
		if(SteamNetworkingSockets()->GetConnectionName((HSteamNetConnection)peer, name, STEAM_BUFFER_SIZE)){
			connection_name += name;	
		}
	}
	return connection_name;
}

// Returns local IP and port that a listen socket created using CreateListenSocketIP is bound to.
String SteamServer::getListenSocketAddress(uint32 socket, bool with_port){
	String socket_address = "";
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingIPAddr address;
		if(SteamNetworkingSockets()->GetListenSocketAddress((HSteamListenSocket)socket, &address)){
			socket_address = getStringFromSteamIP(address);
		}
	}
	return socket_address;
}

// Indicate our desire to be ready participate in authenticated communications. If we are currently not ready, then steps will be taken to obtain the necessary certificates. (This includes a certificate for us, as well as any CA certificates needed to authenticate peers.)
int SteamServer::initAuthentication(){
	if(SteamNetworkingSockets() == NULL){
		return k_ESteamNetworkingAvailability_Unknown;
	}
	return int(SteamNetworkingSockets()->InitAuthentication());
}

// Query our readiness to participate in authenticated communications. A SteamNetAuthenticationStatus_t callback is posted any time this status changes, but you can use this function to query it at any time.
int SteamServer::getAuthenticationStatus(){
	if(SteamNetworkingSockets() == NULL){
		return k_ESteamNetworkingAvailability_Unknown;
	}
	return int(SteamNetworkingSockets()->GetAuthenticationStatus(NULL));
}

// Call this when you receive a ticket from your backend / matchmaking system. Puts the ticket into a persistent cache, and optionally returns the parsed ticket.
//Dictionary SteamServer::receivedRelayAuthTicket(){
//	Dictionary ticket;
//	if(SteamNetworkingSockets() != NULL){
//		SteamDatagramRelayAuthTicket parsed_ticket;
//		PoolByteArray incoming_ticket;
//		incoming_ticket.resize(512);		
//		if(SteamNetworkingSockets()->ReceivedRelayAuthTicket(incoming_ticket.write().ptr(), 512, &parsed_ticket)){
//			char game_server;
//			parsed_ticket.m_identityGameserver.ToString(&game_server, 128);
//			ticket["game_server"] = game_server;
//			char authorized_client;
//			parsed_ticket.m_identityAuthorizedClient.ToString(&authorized_client, 128);
//			ticket["authorized_client"] = authorized_client;
//			ticket["public_ip"] = parsed_ticket.m_unPublicIP;		// uint32
//			ticket["expiry"] = parsed_ticket.m_rtimeTicketExpiry;	// RTime32
//			ticket["routing"] = parsed_ticket.m_routing.GetPopID();			// SteamDatagramHostAddress
//			ticket["app_id"] = parsed_ticket.m_nAppID;				// uint32
//			ticket["restrict_to_v_port"] = parsed_ticket.m_nRestrictToVirtualPort;	// int
//			ticket["number_of_extras"] = parsed_ticket.m_nExtraFields;		// int
//			ticket["extra_fields"] = parsed_ticket.m_vecExtraFields;		// ExtraField
//		}
//	}
//	return ticket;
//}

// Search cache for a ticket to talk to the server on the specified virtual port. If found, returns the number of seconds until the ticket expires, and optionally the complete cracked ticket. Returns 0 if we don't have a ticket.
//int SteamServer::findRelayAuthTicketForServer(int port){
//	int expires_in_seconds = 0;
//	if(SteamNetworkingSockets() != NULL){
//		expires_in_seconds = SteamNetworkingSockets()->FindRelayAuthTicketForServer(game_server, port, &relay_auth_ticket);
//	}
//	return expires_in_seconds;
//}



// Returns the value of the SDR_LISTEN_PORT environment variable. This is the UDP server your server will be listening on. This will configured automatically for you in production environments.
uint16 SteamServer::getHostedDedicatedServerPort(){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->GetHostedDedicatedServerPort();
}

// Returns 0 if SDR_LISTEN_PORT is not set. Otherwise, returns the data center the server is running in. This will be k_SteamDatagramPOPID_dev in non-production envirionment.
uint32 SteamServer::getHostedDedicatedServerPOPId(){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	return SteamNetworkingSockets()->GetHostedDedicatedServerPOPID();
}

// Return info about the hosted server. This contains the PoPID of the server, and opaque routing information that can be used by the relays to send traffic to your server.
//int SteamServer::getHostedDedicatedServerAddress(){
//	int result = 2;
//	if(SteamNetworkingSockets() != NULL){
//		result = SteamNetworkingSockets()->GetHostedDedicatedServerAddress(&hosted_address);
//	}
//	return result;
//}

// Create a listen socket on the specified virtual port. The physical UDP port to use will be determined by the SDR_LISTEN_PORT environment variable. If a UDP port is not configured, this call will fail.
uint32 SteamServer::createHostedDedicatedServerListenSocket(int port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	uint32 listen_socket = SteamGameServerNetworkingSockets()->CreateHostedDedicatedServerListenSocket(port, options.size(), convertOptionsArray(options));
	return listen_socket;
}

// Generate an authentication blob that can be used to securely login with your backend, using SteamDatagram_ParseHostedServerLogin. (See steamdatagram_gamecoordinator.h)
//int SteamServer::getGameCoordinatorServerLogin(String app_data){
//	int result = 2;
//	if(SteamNetworkingSockets() != NULL){	
//		SteamDatagramGameCoordinatorServerLogin *server_login = new SteamDatagramGameCoordinatorServerLogin;
//		server_login->m_cbAppData = app_data.size();
//		strcpy(server_login->m_appData, app_data.utf8().get_data());
//		int signed_blob = k_cbMaxSteamDatagramGameCoordinatorServerLoginSerialized;
//		routing_blob.resize(signed_blob);
//		result = SteamNetworkingSockets()->GetGameCoordinatorServerLogin(server_login, &signed_blob, routing_blob.write().ptr());
//		delete server_login;
//	}
//	return result;
//}

// Returns a small set of information about the real-time state of the connection and the queue status of each lane.
Dictionary SteamServer::getConnectionRealTimeStatus(uint32 connection, int lanes, bool get_status){
	// Create the dictionary for returning
	Dictionary real_time_status;
	if(SteamNetworkingSockets() != NULL){
		SteamNetConnectionRealTimeStatus_t this_status;
		SteamNetConnectionRealTimeLaneStatus_t *lanes_array = new SteamNetConnectionRealTimeLaneStatus_t[lanes];
		int result = SteamNetworkingSockets()->GetConnectionRealTimeStatus((HSteamNetConnection)connection, &this_status, lanes, lanes_array);
		// Append the status
		real_time_status["response"] = result;
		// If the result is good, get more data
		if(result == 0){
			// Get the connection status if requested
			Dictionary connection_status;
			if(get_status){
				connection_status["state"] = this_status.m_eState;
				connection_status["ping"] = this_status.m_nPing;
				connection_status["local_quality"] = this_status.m_flConnectionQualityLocal;
				connection_status["remote_quality"] = this_status.m_flConnectionQualityRemote;
				connection_status["packets_out_per_second"] = this_status.m_flOutPacketsPerSec;
				connection_status["bytes_out_per_second"] = this_status.m_flOutBytesPerSec;
				connection_status["packets_in_per_second"] = this_status.m_flInPacketsPerSec;
				connection_status["bytes_in_per_second"] = this_status.m_flInBytesPerSec;
				connection_status["send_rate"] = this_status.m_nSendRateBytesPerSecond;
				connection_status["pending_unreliable"] = this_status.m_cbPendingUnreliable;
				connection_status["pending_reliable"] = this_status.m_cbPendingReliable;
				connection_status["sent_unacknowledged_reliable"] = this_status.m_cbSentUnackedReliable;
				connection_status["queue_time"] = (uint64_t)this_status.m_usecQueueTime;
			}
			real_time_status["connection_status"] = connection_status;
			// Get the lane information
			Array lanes_status;
			for(int i = 0; i < lanes; i++){
				Dictionary lane_status;
				lane_status["pending_unreliable"] = lanes_array[i].m_cbPendingUnreliable;
				lane_status["pending_reliable"] = lanes_array[i].m_cbPendingReliable;
				lane_status["sent_unacknowledged_reliable"] = lanes_array[i].m_cbSentUnackedReliable;
				lane_status["queue_time"] = (uint64_t)lanes_array[i].m_usecQueueTime;
				lanes_status.append(lane_status);
			}
			delete[] lanes_array;
			real_time_status["lanes_status"] = lanes_status;
		}
	}
	return real_time_status;
}

// Configure multiple outbound messages streams ("lanes") on a connection, and control head-of-line blocking between them.
// Messages within a given lane are always sent in the order they are queued, but messages from different lanes may be sent out of order.
// Each lane has its own message number sequence.  The first message sent on each lane will be assigned the number 1.
int SteamServer::configureConnectionLanes(uint32 connection, int lanes, Array priorities, Array weights) {
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	// Convert the priorities array to an int
	int *lane_priorities = new int[lanes];
	for(int i = 0; i < lanes; i++){
		lane_priorities[i] = priorities[i];
	}
	// Convert the weights array to an int
	uint16 *lane_weights = new uint16[lanes];
	for(int i = 0; i < lanes; i++){
		lane_weights[i] = weights[i];
	}
	int result = SteamNetworkingSockets()->ConfigureConnectionLanes((HSteamNetConnection)connection, lanes, lane_priorities, lane_weights);
	delete[] lane_priorities;
	delete[] lane_weights;
	return result;
}

// Certificate provision by the application. On Steam, we normally handle all this automatically and you will not need to use these advanced functions.
Dictionary SteamServer::getCertificateRequest(){
	Dictionary cert_information;
	if(SteamNetworkingSockets() != NULL){
		int *certificate = new int[512];
		int cert_size = 0;
		SteamNetworkingErrMsg error_message;
		if(SteamNetworkingSockets()->GetCertificateRequest(&cert_size, &certificate, error_message)){
			cert_information["certificate"] = certificate;
			cert_information["cert_size"] = cert_size;
			cert_information["error_message"] = error_message;
		}
		delete[] certificate;
	}
	return cert_information;
}

// Set the certificate. The certificate blob should be the output of SteamDatagram_CreateCert.
Dictionary SteamServer::setCertificate(const PoolByteArray& certificate){
	Dictionary certificate_data;
	if(SteamNetworkingSockets() != NULL){
		bool success = false;
		SteamNetworkingErrMsg error_message;
		success = SteamNetworkingSockets()->SetCertificate((void*)certificate.read().ptr(), certificate.size(), error_message);
		if(success){
			certificate_data["response"] = success;
			certificate_data["error"] = error_message;
		}
	}
	return certificate_data;
}

// Reset the identity associated with this instance. Any open connections are closed.  Any previous certificates, etc are discarded.
// You can pass a specific identity that you want to use, or you can pass NULL, in which case the identity will be invalid until you set it using SetCertificate.
// NOTE: This function is not actually supported on Steam!  It is included for use on other platforms where the active user can sign out and a new user can sign in.
void SteamServer::resetIdentity(uint64_t remote_steam_id){
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingIdentity resetting_identity = getIdentityFromSteamID(remote_steam_id);
		SteamNetworkingSockets()->ResetIdentity(&resetting_identity);
	}
}

// Invoke all callback functions queued for this interface. See k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, etc.
// You don't need to call this if you are using Steam's callback dispatch mechanism (SteamAPI_RunCallbacks and SteamGameserver_RunCallbacks).
void SteamServer::runNetworkingCallbacks(){
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingSockets()->RunCallbacks();		
	}
}

// Begin asynchronous process of allocating a fake IPv4 address that other peers can use to contact us via P2P.
// IP addresses returned by this function are globally unique for a given appid.
// Returns false if a request was already in progress, true if a new request was started.
// A SteamNetworkingFakeIPResult_t will be posted when the request completes.
bool SteamServer::beginAsyncRequestFakeIP(int num_ports){
	if(SteamNetworkingSockets() == NULL){
		return false;
	}
	return SteamNetworkingSockets()->BeginAsyncRequestFakeIP(num_ports);
}

// Return info about the FakeIP and port(s) that we have been assigned, if any.
// idxFirstPort is currently reserved and must be zero. Make sure and check SteamNetworkingFakeIPResult_t::m_eResult
Dictionary SteamServer::getFakeIP(int first_port){
	// Create the return dictionary
	Dictionary fake_ip;
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingFakeIPResult_t fake_ip_result;
		SteamNetworkingSockets()->GetFakeIP(first_port, &fake_ip_result);
		// Populate the dictionary
		fake_ip["result"] = fake_ip_result.m_eResult;
		fake_ip["identity_type"] = fake_ip_result.m_identity.m_eType;
		fake_ip["ip"] = getStringFromIP(fake_ip_result.m_unIP);
		char ports[8];
		for (size_t i = 0; i < sizeof(fake_ip_result.m_unPorts) / sizeof(fake_ip_result.m_unPorts[0]); i++){
			ports[i] = fake_ip_result.m_unPorts[i];
		}
		fake_ip["ports"] = ports;
	}
	return fake_ip;
}

// Create a listen socket that will listen for P2P connections sent to our FakeIP.
// A peer can initiate connections to this listen socket by calling ConnectByIPAddress.
uint32 SteamServer::createListenSocketP2PFakeIP(int fake_port, Array options){
	if(SteamNetworkingSockets() == NULL){
		return 0;
	}
	uint32 listen_socket = SteamNetworkingSockets()->CreateListenSocketP2PFakeIP(fake_port, options.size(), convertOptionsArray(options));
	return listen_socket;
}

// If the connection was initiated using the "FakeIP" system, then we we can get an IP address for the remote host.  If the remote host had a global FakeIP at the time the connection was established, this function will return that global IP.
// Otherwise, a FakeIP that is unique locally will be allocated from the local FakeIP address space, and that will be returned.
Dictionary SteamServer::getRemoteFakeIPForConnection(uint32 connection){
	// Create the return dictionary
	Dictionary this_fake_address;
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingIPAddr fake_address;
		int result = SteamNetworkingSockets()->GetRemoteFakeIPForConnection((HSteamNetConnection)connection, &fake_address);
		// Send back the data
		this_fake_address["result"] = result;
		this_fake_address["ip_address"] = getStringFromSteamIP(fake_address);
		this_fake_address["port"] = fake_address.m_port;
		this_fake_address["ip_type"] = fake_address.GetFakeIPType();
		}

	return this_fake_address;
}

// Get an interface that can be used like a UDP port to send/receive datagrams to a FakeIP address.
// This is intended to make it easy to port existing UDP-based code to take advantage of SDR.
// To create a "client" port (e.g. the equivalent of an ephemeral UDP port) pass -1.
void SteamServer::createFakeUDPPort(int fake_server_port_index){
	if(SteamNetworkingSockets() != NULL){
		SteamNetworkingSockets()->CreateFakeUDPPort(fake_server_port_index);
	}
}


///// NETWORKING UTILS
/////////////////////////////////////////////////
//
// If you know that you are going to be using the relay network (for example, because you anticipate making P2P connections), call this to initialize the relay network. If you do not call this, the initialization will be delayed until the first time you use a feature that requires access to the relay network, which will delay that first access.
void SteamServer::initRelayNetworkAccess(){
	if(SteamNetworkingUtils() != NULL){
		SteamNetworkingUtils()->InitRelayNetworkAccess();
	}
}

// Fetch current status of the relay network.  If you want more details, you can pass a non-NULL value.
int SteamServer::getRelayNetworkStatus(){
	if(SteamNetworkingUtils() == NULL){
		return k_ESteamNetworkingAvailability_Unknown;
	}
	return int(SteamNetworkingUtils()->GetRelayNetworkStatus(NULL));
}

// Return location info for the current host. Returns the approximate age of the data, in seconds, or -1 if no data is available.
Dictionary SteamServer::getLocalPingLocation(){
	Dictionary ping_location;
	if(SteamNetworkingUtils() != NULL){
		SteamNetworkPingLocation_t location;
		float age = SteamNetworkingUtils()->GetLocalPingLocation(location);
		// Populate the dictionary
		PoolByteArray data;
		data.resize(512);
		uint8_t* output_data = data.write().ptr();
		for(int j = 0; j < 512; j++){
			output_data[j] = location.m_data[j];
		}
		ping_location["age"] = age;
		ping_location["location"] = data;
	}
	return ping_location;
}

// Estimate the round-trip latency between two arbitrary locations, in milliseconds. This is a conservative estimate, based on routing through the relay network. For most basic relayed connections, this ping time will be pretty accurate, since it will be based on the route likely to be actually used.
int SteamServer::estimatePingTimeBetweenTwoLocations(PoolByteArray location1, PoolByteArray location2){
	if(SteamNetworkingUtils() == NULL){
		return 0;
	}
	// Add these locations to ping structs
	SteamNetworkPingLocation_t ping_location1;
	SteamNetworkPingLocation_t ping_location2;
	uint8_t* input_location_1 = (uint8*) location1.read().ptr();
	for(int j = 0; j < 512; j++){
		ping_location1.m_data[j] = input_location_1[j];
	}
	uint8_t* input_location_2 = (uint8*) location2.read().ptr();
	for(int j = 0; j < 512; j++){
		ping_location2.m_data[j] = (uint8) input_location_2[j];
	}
	return SteamNetworkingUtils()->EstimatePingTimeBetweenTwoLocations(ping_location1, ping_location2);
}

// Same as EstimatePingTime, but assumes that one location is the local host. This is a bit faster, especially if you need to calculate a bunch of these in a loop to find the fastest one.
int SteamServer::estimatePingTimeFromLocalHost(PoolByteArray location){
	if(SteamNetworkingUtils() == NULL){
		return 0;
	}
	// Add this location to ping struct
	SteamNetworkPingLocation_t ping_location;
	uint8_t* input_location = (uint8*) location.read().ptr();
	for(int j = 0; j < 512; j++){
		ping_location.m_data[j] = input_location[j];
	}
	return SteamNetworkingUtils()->EstimatePingTimeFromLocalHost(ping_location);
}

// Convert a ping location into a text format suitable for sending over the wire. The format is a compact and human readable. However, it is subject to change so please do not parse it yourself. Your buffer must be at least k_cchMaxSteamNetworkingPingLocationString bytes.
String SteamServer::convertPingLocationToString(PoolByteArray location){
	String location_string = "";
	if(SteamNetworkingUtils() != NULL){
		char *buffer = new char[512];
		// Add this location to ping struct
		SteamNetworkPingLocation_t ping_location;
		uint8_t* input_location = (uint8*) location.read().ptr();
		for(int j = 0; j < 512; j++){
			ping_location.m_data[j] = input_location[j];
		}
		SteamNetworkingUtils()->ConvertPingLocationToString(ping_location, buffer, k_cchMaxSteamNetworkingPingLocationString);
		location_string += buffer;
		delete[] buffer;
	}
	return location_string;
}

// Parse back SteamNetworkPingLocation_t string. Returns false if we couldn't understand the string.
Dictionary SteamServer::parsePingLocationString(String location_string){
	Dictionary parse_string;
	if(SteamNetworkingUtils() != NULL){
		SteamNetworkPingLocation_t result;
		bool success = SteamNetworkingUtils()->ParsePingLocationString(location_string.utf8().get_data(), result);
		// Populate the dictionary
		PoolByteArray data;
		data.resize(512);
		uint8_t* output_data = data.write().ptr();
		for(int j = 0; j < 512; j++){
			output_data[j] = result.m_data[j];
		}
		parse_string["success"] = success;
		parse_string["ping_location"] = data;
	}
	return parse_string;
}

// Check if the ping data of sufficient recency is available, and if it's too old, start refreshing it.
bool SteamServer::checkPingDataUpToDate(float max_age_in_seconds){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->CheckPingDataUpToDate(max_age_in_seconds);
}

// Fetch ping time of best available relayed route from this host to the specified data center.
Dictionary SteamServer::getPingToDataCenter(uint32 pop_id){
	Dictionary data_center_ping;
	if(SteamNetworkingUtils() != NULL){
		SteamNetworkingPOPID via_relay_pop;
		int ping = SteamNetworkingUtils()->GetPingToDataCenter((SteamNetworkingPOPID)pop_id, &via_relay_pop);
		// Populate the dictionary
		data_center_ping["pop_relay"] = via_relay_pop;
		data_center_ping["ping"] = ping;
	}
	return data_center_ping;
}

// Get *direct* ping time to the relays at the point of presence.
int SteamServer::getDirectPingToPOP(uint32 pop_id){
	if(SteamNetworkingUtils() == NULL){
		return 0;
	}
	return SteamNetworkingUtils()->GetDirectPingToPOP((SteamNetworkingPOPID)pop_id);
}

// Get number of network points of presence in the config
int SteamServer::getPOPCount(){
	if(SteamNetworkingUtils() == NULL){
		return 0;
	}
	return SteamNetworkingUtils()->GetPOPCount();
}

// Get list of all POP IDs. Returns the number of entries that were filled into your list.
Array SteamServer::getPOPList(){
	Array pop_list;
	if(SteamNetworkingUtils() != NULL){
		SteamNetworkingPOPID *list = new SteamNetworkingPOPID[256];
		int pops = SteamNetworkingUtils()->GetPOPList(list, 256);
		// Iterate and add
		for(int i = 0; i < pops; i++){
			int pop_id = list[i];
			pop_list.append(pop_id);
		}
		delete[] list;
	}
	return pop_list;
}

// Set a configuration value.
//bool SteamServer::setConfigValue(int setting, int scope_type, uint32_t connection_handle, int data_type, auto value){
//	if(SteamNetworkingUtils() == NULL){
//		return false;
//	}
//	return SteamNetworkingUtils()->SetConfigValue((ESteamNetworkingConfigValue)setting, (ESteamNetworkingConfigScope)scope_type, connection_handle, (ESteamNetworkingConfigDataType)data_type, value);
//}

// Get a configuration value.
Dictionary SteamServer::getConfigValue(int config_value, int scope_type, uint32_t connection_handle){
	Dictionary config_info;
	if(SteamNetworkingUtils() != NULL){
		ESteamNetworkingConfigDataType data_type;
		size_t buffer_size;
		PoolByteArray config_result;
		int result = SteamNetworkingUtils()->GetConfigValue((ESteamNetworkingConfigValue)config_value, (ESteamNetworkingConfigScope)scope_type, connection_handle, &data_type, &config_result, &buffer_size);
		// Populate the dictionary
		config_info["result"] = result;
		config_info["type"] = data_type;
		config_info["value"] = config_result;
		config_info["buffer"] = (uint64_t)buffer_size;
	}
	return config_info;
}

// Returns info about a configuration value.
Dictionary SteamServer::getConfigValueInfo(int config_value){
	Dictionary config_info;
	if(SteamNetworkingUtils() != NULL){
		ESteamNetworkingConfigDataType data_type;
		ESteamNetworkingConfigScope scope;
		if(SteamNetworkingUtils()->GetConfigValueInfo((ESteamNetworkingConfigValue)config_value, &data_type, &scope)){
			// Populate the dictionary
			config_info["type"] = data_type;
			config_info["scope"] = scope;
		}
	}
	return config_info;
}

// The following functions are handy shortcuts for common use cases.
bool SteamServer::setGlobalConfigValueInt32(int config, int32 value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetGlobalConfigValueInt32((ESteamNetworkingConfigValue)config, value);
}
bool SteamServer::setGlobalConfigValueFloat(int config, float value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetGlobalConfigValueFloat((ESteamNetworkingConfigValue)config, value);
}
bool SteamServer::setGlobalConfigValueString(int config, String value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetGlobalConfigValueString((ESteamNetworkingConfigValue)config, value.utf8().get_data());
}
bool SteamServer::setConnectionConfigValueInt32(uint32 connection, int config, int32 value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetConnectionConfigValueInt32(connection, (ESteamNetworkingConfigValue)config, value);
}
bool SteamServer::setConnectionConfigValueFloat(uint32 connection, int config, float value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetConnectionConfigValueFloat(connection, (ESteamNetworkingConfigValue)config, value);
}

bool SteamServer::setConnectionConfigValueString(uint32 connection, int config, String value){
	if(SteamNetworkingUtils() == NULL){
		return false;
	}
	return SteamNetworkingUtils()->SetConnectionConfigValueString(connection, (ESteamNetworkingConfigValue)config, value.utf8().get_data());
}

// A general purpose high resolution local timer with the following properties: Monotonicity is guaranteed. The initial value will be at least 24*3600*30*1e6, i.e. about 30 days worth of microseconds. In this way, the timestamp value of 0 will always be at least "30 days ago". Also, negative numbers will never be returned. Wraparound / overflow is not a practical concern.
uint64_t SteamServer::getLocalTimestamp(){
	if(SteamNetworkingUtils() == NULL){
		return 0;
	}
	return SteamNetworkingUtils()->GetLocalTimestamp();
}


///// UGC
/////////////////////////////////////////////////
//
// Adds a dependency between the given item and the appid. This list of dependencies can be retrieved by calling GetAppDependencies.
// This is a soft-dependency that is displayed on the web. It is up to the application to determine whether the item can actually be used or not.
void SteamServer::addAppDependency(uint64_t published_file_id, uint32_t app_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		AppId_t app = (uint32_t)app_id;
		SteamAPICall_t api_call = SteamUGC()->AddAppDependency(file_id, app);
		callResultAddAppDependency.Set(api_call, this, &SteamServer::add_app_dependency_result);
	}
}

bool SteamServer::addContentDescriptor(uint64_t update_handle, int descriptor_id){
	if(SteamUGC() == NULL){
		return false;
	}
	return SteamUGC()->AddContentDescriptor((UGCUpdateHandle_t)update_handle, (EUGCContentDescriptorID)descriptor_id);
}

// Adds a workshop item as a dependency to the specified item. If the nParentPublishedFileID item is of type k_EWorkshopFileTypeCollection, than the nChildPublishedFileID is simply added to that collection.
// Otherwise, the dependency is a soft one that is displayed on the web and can be retrieved via the ISteamUGC API using a combination of the m_unNumChildren member variable of the SteamUGCDetails_t struct and GetQueryUGCChildren.
void SteamServer::addDependency(uint64_t published_file_id, uint64_t child_published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t parent = (uint64_t)published_file_id;
		PublishedFileId_t child = (uint64_t)child_published_file_id;
		SteamAPICall_t api_call = SteamUGC()->AddDependency(parent, child);
		callResultAddUGCDependency.Set(api_call, this, &SteamServer::add_ugc_dependency_result);
	}
}

// Adds a excluded tag to a pending UGC Query. This will only return UGC without the specified tag.
bool SteamServer::addExcludedTag(uint64_t query_handle, String tag_name){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->AddExcludedTag(handle, tag_name.utf8().get_data());
}

// Adds a key-value tag pair to an item. Keys can map to multiple different values (1-to-many relationship).
bool SteamServer::addItemKeyValueTag(uint64_t update_handle, String key, String value){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = (uint64_t)update_handle;
	return SteamUGC()->AddItemKeyValueTag(handle, key.utf8().get_data(), value.utf8().get_data());
}

// Adds an additional preview file for the item.
bool SteamServer::addItemPreviewFile(uint64_t query_handle, String preview_file, int type){
	if(SteamUGC() == NULL){
		return false;
	}
	return SteamUGC()->AddItemPreviewFile((UGCQueryHandle_t)query_handle, preview_file.utf8().get_data(), (EItemPreviewType)type);
}

// Adds an additional video preview from YouTube for the item.
bool SteamServer::addItemPreviewVideo(uint64_t query_handle, String video_id){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->AddItemPreviewVideo(handle, video_id.utf8().get_data());
}

// Adds a workshop item to the users favorites list.
void SteamServer::addItemToFavorites(uint32_t app_id, uint64_t published_file_id){
	if(SteamUGC() != NULL){
		AppId_t app = (uint32_t)app_id;
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->AddItemToFavorites(app, file_id);
		callResultFavoriteItemListChanged.Set(api_call, this, &SteamServer::user_favorite_items_list_changed);
	}
}

// Adds a required key-value tag to a pending UGC Query. This will only return workshop items that have a key = pKey and a value = pValue.
bool SteamServer::addRequiredKeyValueTag(uint64_t query_handle, String key, String value){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->AddRequiredKeyValueTag(handle, key.utf8().get_data(), value.utf8().get_data());
}

// Adds a required tag to a pending UGC Query. This will only return UGC with the specified tag.
bool SteamServer::addRequiredTag(uint64_t query_handle, String tag_name){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->AddRequiredTag(handle, tag_name.utf8().get_data());
}

// Adds the requirement that the returned items from the pending UGC Query have at least one of the tags in the given set (logical "or"). For each tag group that is added, at least one tag from each group is required to be on the matching items.
bool SteamServer::addRequiredTagGroup(uint64_t query_handle, Array tag_array){
	bool added_tag_group = false;
	if(SteamUGC() != NULL){
		UGCQueryHandle_t handle = uint64(query_handle);
		SteamParamStringArray_t *tags = new SteamParamStringArray_t();
		tags->m_ppStrings = new const char*[tag_array.size()];
		uint32 strCount = tag_array.size();
		for (uint32 i=0; i < strCount; i++) {
			String str = tag_array[i];
			tags->m_ppStrings[i] = str.utf8().get_data();
		}
		tags->m_nNumStrings = tag_array.size();
		added_tag_group = SteamUGC()->AddRequiredTagGroup(handle, tags);
		delete tags;
	}
	return added_tag_group;
}

// Lets game servers set a specific workshop folder before issuing any UGC commands.
bool SteamServer::initWorkshopForGameServer(uint32_t workshop_depot_id){
	bool initialized_workshop = false;
	if(SteamUGC() != NULL){
		DepotId_t workshop = (uint32_t)workshop_depot_id;
		const char *folder = new char[256];
		initialized_workshop = SteamUGC()->BInitWorkshopForGameServer(workshop, (char*)folder);
		delete[] folder;
	}
	return initialized_workshop;
}

// Creates a new workshop item with no content attached yet.
void SteamServer::createItem(uint32 app_id, int file_type){
	if(SteamUGC() != NULL){
		SteamAPICall_t api_call = SteamUGC()->CreateItem((AppId_t)app_id, (EWorkshopFileType)file_type);
		callResultItemCreate.Set(api_call, this, &SteamServer::item_created);
	}
}

// Query for all matching UGC. You can use this to list all of the available UGC for your app.
uint64_t SteamServer::createQueryAllUGCRequest(int query_type, int matching_type, uint32_t creator_id, uint32_t consumer_id, uint32 page){
	if(SteamUGC() == NULL){
		return 0;
	}
	AppId_t creator = (uint32_t)creator_id;
	AppId_t consumer = (uint32_t)consumer_id;
	UGCQueryHandle_t handle = SteamUGC()->CreateQueryAllUGCRequest((EUGCQuery)query_type, (EUGCMatchingUGCType)matching_type, creator, consumer, page);
	return (uint64_t)handle;
}

// Query for the details of specific workshop items.
uint64_t SteamServer::createQueryUGCDetailsRequest(Array published_file_ids){
	uint64_t this_handle = 0;
	if(SteamUGC() != NULL){
		uint32 fileCount = published_file_ids.size();
		if(fileCount != 0){
			PublishedFileId_t *file_ids = new PublishedFileId_t[fileCount];
			for(uint32 i = 0; i < fileCount; i++){
				file_ids[i] = (uint64_t)published_file_ids[i];
			}
			UGCQueryHandle_t handle = SteamUGC()->CreateQueryUGCDetailsRequest(file_ids, fileCount);
			delete[] file_ids;
			this_handle = (uint64_t)handle;
		}
	}
	return this_handle;
}

// Query UGC associated with a user. You can use this to list the UGC the user is subscribed to amongst other things.
uint64_t SteamServer::createQueryUserUGCRequest(uint64_t steam_id, int list_type, int matching_ugc_type, int sort_order, uint32_t creator_id, uint32_t consumer_id, uint32 page){
	if(SteamUGC() == NULL){
		return 0;
	}
	// Get tue universe ID from the Steam ID
	CSteamID user_id = (uint64)steam_id;
	AccountID_t account = (AccountID_t)user_id.ConvertToUint64();
	AppId_t creator = (int)creator_id;
	AppId_t consumer = (int)consumer_id;
	UGCQueryHandle_t handle = SteamUGC()->CreateQueryUserUGCRequest(account, (EUserUGCList)list_type, (EUGCMatchingUGCType)matching_ugc_type, (EUserUGCListSortOrder)sort_order, creator, consumer, page);
	return (uint64_t)handle;
}

// Deletes the item without prompting the user.
void SteamServer::deleteItem(uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->DeleteItem(file_id);
		callResultDeleteItem.Set(api_call, this, &SteamServer::item_deleted);
	}
}

// Download new or update already installed item. If returns true, wait for DownloadItemResult_t. If item is already installed, then files on disk should not be used until callback received.
// If item is not subscribed to, it will be cached for some time. If bHighPriority is set, any other item download will be suspended and this item downloaded ASAP.
bool SteamServer::downloadItem(uint64_t published_file_id, bool high_priority){
	if(SteamUGC() == NULL){
		return false;
	}
	PublishedFileId_t file_id = (uint64_t)published_file_id;
	return SteamUGC()->DownloadItem(file_id, high_priority);
}

// Get info about a pending download of a workshop item that has k_EItemStateNeedsUpdate set.
Dictionary SteamServer::getItemDownloadInfo(uint64_t published_file_id){
	Dictionary info;
	if(SteamUGC() == NULL){
		return info;
	}
	uint64 downloaded = 0;
	uint64 total = 0;
	info["ret"] = SteamUGC()->GetItemDownloadInfo((PublishedFileId_t)published_file_id, &downloaded, &total);
	if(info["ret"]){
		info["downloaded"] = uint64_t(downloaded);
		info["total"] = uint64_t(total);
	}
	return info;
}

// Gets info about currently installed content on the disc for workshop items that have k_EItemStateInstalled set.
Dictionary SteamServer::getItemInstallInfo(uint64_t published_file_id){
	Dictionary info;
	if(SteamUGC() == NULL){
		info["ret"] = false;
		return info;
	}
	PublishedFileId_t file_id = (uint64_t)published_file_id;
	uint64 sizeOnDisk;
	char folder[1024] = { 0 };
	uint32 timeStamp;
	info["ret"] = SteamUGC()->GetItemInstallInfo((PublishedFileId_t)file_id, &sizeOnDisk, folder, sizeof(folder), &timeStamp);
	if(info["ret"]){
		info["size"] = (int)sizeOnDisk;
		info["folder"] = folder;
		info["foldersize"] = (uint32)sizeof(folder);
		info["timestamp"] = timeStamp;
	}
	return info;
}

// Gets the current state of a workshop item on this client.
uint32 SteamServer::getItemState(uint64_t published_file_id){
	if(SteamUGC() == NULL){
		return 0;
	}
	PublishedFileId_t file_id = (uint64_t)published_file_id;
	return SteamUGC()->GetItemState(file_id);
}

// Gets the progress of an item update.
Dictionary SteamServer::getItemUpdateProgress(uint64_t update_handle){
	Dictionary updateProgress;
	if(SteamUGC() == NULL){
		return updateProgress;
	}
	UGCUpdateHandle_t handle = (uint64_t)update_handle;
	uint64 processed = 0;
	uint64 total = 0;
	EItemUpdateStatus status = SteamUGC()->GetItemUpdateProgress(handle, &processed, &total);
	updateProgress["status"] = status;
	updateProgress["processed"] = uint64_t(processed);
	updateProgress["total"] = uint64_t(total);
	return updateProgress;
}

// Gets the total number of items the current user is subscribed to for the game or application.
uint32 SteamServer::getNumSubscribedItems(){
	if(SteamUser() == NULL){
		return 0;
	}
	return SteamUGC()->GetNumSubscribedItems();
}

// Retrieve the details of an additional preview associated with an individual workshop item after receiving a querying UGC call result.
Dictionary SteamServer::getQueryUGCAdditionalPreview(uint64_t query_handle, uint32 index, uint32 preview_index){
	Dictionary preview;
	if(SteamUGC() == NULL){
		return preview;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	char *url_or_video_id = new char[256];
	char *original_filename = new char[256];
	EItemPreviewType previewType;
	bool success = SteamUGC()->GetQueryUGCAdditionalPreview(handle, index, preview_index, (char*)url_or_video_id, 256, (char*)original_filename, 256, &previewType);
	if(success){
		preview["success"] = success;
		preview["handle"] = (uint64_t)handle;
		preview["index"] = index;
		preview["preview"] = preview_index;
		preview["urlOrVideo"] = url_or_video_id;
		preview["filename"] = original_filename;
		preview["type"] = previewType;
	}
	delete[] url_or_video_id;
	delete[] original_filename;
	return preview;
}

// Retrieve the ids of any child items of an individual workshop item after receiving a querying UGC call result. These items can either be a part of a collection or some other dependency (see AddDependency).
Dictionary SteamServer::getQueryUGCChildren(uint64_t query_handle, uint32 index, uint32_t child_count){
	Dictionary children;
	if(SteamUGC() == NULL){
		return children;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	PoolIntArray child_array;
	child_array.resize(child_count);
	bool success = SteamUGC()->GetQueryUGCChildren(handle, index, (PublishedFileId_t*)child_array.write().ptr(), child_count);
	if(success) {
		Array godot_arr;
		godot_arr.resize(child_count);
		for (uint32_t i = 0; i < child_count; i++) {
			godot_arr[i] = child_array[i];
		}
		
		children["success"] = success;
		children["handle"] = (uint64_t)handle;
		children["index"] = index;
		children["children"] = godot_arr;
	}
	return children;
}

Dictionary SteamServer::getQueryUGCContentDescriptors(uint64_t query_handle, uint32 index, uint32_t max_entries){
	Dictionary descriptors;
	if(SteamUGC() != NULL){
		UGCQueryHandle_t handle = (uint64_t)query_handle;
		PoolIntArray descriptor_list;
		descriptor_list.resize(max_entries);
		uint32_t result = SteamUGC()->GetQueryUGCContentDescriptors(handle, index, (EUGCContentDescriptorID*)descriptor_list.write().ptr(), max_entries);
		Array descriptor_array;
		descriptor_array.resize(max_entries);
		for(uint32_t i = 0; i < max_entries; i++){
			descriptor_array[i] = descriptor_list[i];
		}
		descriptors["result"] = result;
		descriptors["handle"] = (uint64_t)handle;
		descriptors["index"] = index;
		descriptors["descriptors"] = descriptor_array;
	}
	return descriptors;
}

// Retrieve the details of a key-value tag associated with an individual workshop item after receiving a querying UGC call result.
Dictionary SteamServer::getQueryUGCKeyValueTag(uint64_t query_handle, uint32 index, uint32 key_value_tag_index){
	Dictionary tag;
	if(SteamUGC() == NULL){
		return tag;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	char *key = new char[256];
	char *value = new char[256];
	bool success = SteamUGC()->GetQueryUGCKeyValueTag(handle, index, key_value_tag_index, (char*)key, 256, (char*)value, 256);
	if(success){
		tag["success"] = success;
		tag["handle"] = (uint64_t)handle;
		tag["index"] = index;
		tag["tag"] = key_value_tag_index;
		tag["key"] = key;
		tag["value"] = value;
	}
	delete[] key;
	delete[] value;
	return tag;
}

// Retrieve the developer set metadata of an individual workshop item after receiving a querying UGC call result.
String SteamServer::getQueryUGCMetadata(uint64_t query_handle, uint32 index){
	String query_ugc_metadata = "";
	if(SteamUGC() != NULL){
		UGCQueryHandle_t handle = (uint64_t)query_handle;
		char *metadata = new char[5000];
		bool success = SteamUGC()->GetQueryUGCMetadata(handle, index, (char*)metadata, 5000);
		if(success){
			query_ugc_metadata = metadata;
		}
		delete[] metadata;
	}
	return query_ugc_metadata;
}

// Retrieve the number of additional previews of an individual workshop item after receiving a querying UGC call result.
uint32 SteamServer::getQueryUGCNumAdditionalPreviews(uint64_t query_handle, uint32 index){
	if(SteamUser() == NULL){
		return 0;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->GetQueryUGCNumAdditionalPreviews(handle, index);
}

// Retrieve the number of key-value tags of an individual workshop item after receiving a querying UGC call result.
uint32 SteamServer::getQueryUGCNumKeyValueTags(uint64_t query_handle, uint32 index){
	if(SteamUser() == NULL){
		return 0;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->GetQueryUGCNumKeyValueTags(handle, index);
}

// Retrieve the number of tags for an individual workshop item after receiving a querying UGC call result. You should call this in a loop to get the details of all the workshop items returned.
uint32 SteamServer::getQueryUGCNumTags(uint64_t query_handle, uint32 index){
	if(SteamUGC() == NULL){
		return 0;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->GetQueryUGCNumTags(handle, index);
}

// Retrieve the URL to the preview image of an individual workshop item after receiving a querying UGC call result.
String SteamServer::getQueryUGCPreviewURL(uint64_t query_handle, uint32 index){
	String query_ugc_preview_url = "";
	if(SteamUGC() != NULL){
		UGCQueryHandle_t handle = (uint64_t)query_handle;
		char *url = new char[256];
		bool success = SteamUGC()->GetQueryUGCPreviewURL(handle, index, (char*)url, 256);
		if(success){
			query_ugc_preview_url = url;
		}
		delete[] url;
	}
	return query_ugc_preview_url;
}

// Retrieve the details of an individual workshop item after receiving a querying UGC call result.
Dictionary SteamServer::getQueryUGCResult(uint64_t query_handle, uint32 index) {
	Dictionary ugc_result;
	if (SteamUGC() == NULL) {
		return ugc_result;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	SteamUGCDetails_t query_details;
	bool success = SteamUGC()->GetQueryUGCResult(handle, index, &query_details);
	if (success) {
		ugc_result["result"] = (uint64_t)query_details.m_eResult;
		ugc_result["file_id"] = (uint64_t)query_details.m_nPublishedFileId;
		ugc_result["file_type"] = (uint64_t)query_details.m_eFileType;
		ugc_result["creator_app_id"] = (uint32_t)query_details.m_nCreatorAppID;
		ugc_result["consumer_app_id"] = (uint32_t)query_details.m_nConsumerAppID;
		ugc_result["title"] = String(query_details.m_rgchTitle);
		ugc_result["description"] = String(query_details.m_rgchDescription);
		ugc_result["steam_id_owner"] = (uint64_t)query_details.m_ulSteamIDOwner;
		ugc_result["time_created"] = query_details.m_rtimeCreated;
		ugc_result["time_updated"] = query_details.m_rtimeUpdated;
		ugc_result["time_added_to_user_list"] = query_details.m_rtimeAddedToUserList;
		ugc_result["visibility"] = (uint64_t)query_details.m_eVisibility;
		ugc_result["banned"] = query_details.m_bBanned;
		ugc_result["accepted_for_use"] = query_details.m_bAcceptedForUse;
		ugc_result["tags_truncated"] = query_details.m_bTagsTruncated;
		ugc_result["tags"] = query_details.m_rgchTags;
		ugc_result["handle_file"] = (uint64_t)query_details.m_hFile;
		ugc_result["handle_preview_file"] = (uint64_t)query_details.m_hPreviewFile;
		ugc_result["file_name"] = query_details.m_pchFileName;
		ugc_result["file_size"] = query_details.m_nFileSize;
		ugc_result["preview_file_size"] = query_details.m_nPreviewFileSize;
		ugc_result["url"] = query_details.m_rgchURL;
		ugc_result["votes_up"] = query_details.m_unVotesUp;
		ugc_result["votes_down"] = query_details.m_unVotesDown;
		ugc_result["score"] = query_details.m_flScore;
		ugc_result["num_children"] = query_details.m_unNumChildren;
	}
	return ugc_result;
}

// Retrieve various statistics of an individual workshop item after receiving a querying UGC call result.
Dictionary SteamServer::getQueryUGCStatistic(uint64_t query_handle, uint32 index, int stat_type){
	Dictionary ugc_stat;
	if (SteamUGC() == NULL) {
		return ugc_stat;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	uint64 value = 0;
	bool success = SteamUGC()->GetQueryUGCStatistic(handle, index, (EItemStatistic)stat_type, &value);
	if (success) {
		ugc_stat["success"] = success;
		ugc_stat["handle"] = (uint64_t)handle;
		ugc_stat["index"] = index;
		ugc_stat["type"] = stat_type;
		ugc_stat["value"] = (uint64_t)value;
	}
	return ugc_stat;
}

// Retrieve the "nth" tag associated with an individual workshop item after receiving a querying UGC call result.
// You should call this in a loop to get the details of all the workshop items returned.
String SteamServer::getQueryUGCTag(uint64_t query_handle, uint32 index, uint32 tag_index){
	if(SteamUGC() == NULL){
		return "";
	}
	// Set a default tag to return
	char *tag = new char[64];
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	SteamUGC()->GetQueryUGCTag(handle, index, tag_index, tag, 64);
	tag[63] = '\0';
	String tag_name = tag;
	delete[] tag;
	return tag_name;
}

// Retrieve the "nth" display string (usually localized) for a tag, which is associated with an individual workshop item after receiving a querying UGC call result.
// You should call this in a loop to get the details of all the workshop items returned.
String SteamServer::getQueryUGCTagDisplayName(uint64_t query_handle, uint32 index, uint32 tag_index){
	if(SteamUGC() == NULL){
		return "";
	}
	// Set a default tag name to return
	char *tag = new char[256];
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	SteamUGC()->GetQueryUGCTagDisplayName(handle, index, tag_index, tag, 256);
	tag[255] = '\0';
	String tagDisplay = tag;
	delete[] tag;
	return tagDisplay;
}

// Gets a list of all of the items the current user is subscribed to for the current game.
Array SteamServer::getSubscribedItems(){
	if(SteamUGC() == NULL){
		return Array();
	}
	Array subscribed;
	uint32 num_items = SteamUGC()->GetNumSubscribedItems();
	PublishedFileId_t *items = new PublishedFileId_t[num_items];
	uint32 item_list = SteamUGC()->GetSubscribedItems(items, num_items);
	for(uint32 i = 0; i < item_list; i++){
		subscribed.append((uint64_t)items[i]);
	}
	delete[] items;
	return subscribed;
}

// Return the user's community content descriptor preferences
// Information is unclear how this actually works so here goes nothing!
Array SteamServer::getUserContentDescriptorPreferences(uint32 max_entries){
	Array descriptors;
	if(SteamUGC() != NULL){
		EUGCContentDescriptorID *descriptor_list = new EUGCContentDescriptorID[max_entries];
		uint32 num_descriptors = SteamUGC()->GetUserContentDescriptorPreferences(descriptor_list, max_entries);
		for(uint32 i = 0; i < num_descriptors; i++){
			descriptors.append(descriptor_list[i]);
		}
	}
	return descriptors;
}

// Gets the users vote status on a workshop item.
void SteamServer::getUserItemVote(uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->GetUserItemVote(file_id);
		callResultGetUserItemVote.Set(api_call, this, &SteamServer::get_item_vote_result);
	}
}

// Releases a UGC query handle when you are done with it to free up memory.
bool SteamServer::releaseQueryUGCRequest(uint64_t query_handle){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->ReleaseQueryUGCRequest(handle);
}

// Removes the dependency between the given item and the appid. This list of dependencies can be retrieved by calling GetAppDependencies.
void SteamServer::removeAppDependency(uint64_t published_file_id, uint32_t app_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		AppId_t app = (uint32_t)app_id;
		SteamAPICall_t api_call = SteamUGC()->RemoveAppDependency(file_id, app);
		callResultRemoveAppDependency.Set(api_call, this, &SteamServer::remove_app_dependency_result);
	}
}

bool SteamServer::removeContentDescriptor(uint64_t update_handle, int descriptor_id){
	if(SteamUGC() == NULL){
		return false;
	}
	return SteamUGC()->RemoveContentDescriptor((UGCUpdateHandle_t)update_handle, (EUGCContentDescriptorID)descriptor_id);
}

// Removes a workshop item as a dependency from the specified item.
void SteamServer::removeDependency(uint64_t published_file_id, uint64_t child_published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		PublishedFileId_t childID = (uint64_t)child_published_file_id;
		SteamAPICall_t api_call = SteamUGC()->RemoveDependency(file_id, childID);
		callResultRemoveUGCDependency.Set(api_call, this, &SteamServer::remove_ugc_dependency_result);
	}
}

// Removes a workshop item from the users favorites list.
void SteamServer::removeItemFromFavorites(uint32_t app_id, uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		AppId_t app = (uint32_t)app_id;
		SteamAPICall_t api_call = SteamUGC()->RemoveItemFromFavorites(app, file_id);
		callResultFavoriteItemListChanged.Set(api_call, this, &SteamServer::user_favorite_items_list_changed);
	}
}

// Removes an existing key value tag from an item.
bool SteamServer::removeItemKeyValueTags(uint64_t update_handle, String key){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->RemoveItemKeyValueTags(handle, key.utf8().get_data());
}

// Removes an existing preview from an item.
bool SteamServer::removeItemPreview(uint64_t update_handle, uint32 index){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->RemoveItemPreview(handle, index);
}

// Send a UGC query to Steam.
void SteamServer::sendQueryUGCRequest(uint64_t update_handle){
	if(SteamUGC() != NULL){
		UGCUpdateHandle_t handle = uint64(update_handle);
		SteamAPICall_t api_call = SteamUGC()->SendQueryUGCRequest(handle);
		callResultUGCQueryCompleted.Set(api_call, this, &SteamServer::ugc_query_completed);
	}
}

// Sets whether results will be returned from the cache for the specific period of time on a pending UGC Query.
bool SteamServer::setAllowCachedResponse(uint64_t update_handle, uint32 max_age_seconds){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetAllowCachedResponse(handle, max_age_seconds);
}

// Sets to only return items that have a specific filename on a pending UGC Query.
bool SteamServer::setCloudFileNameFilter(uint64_t update_handle, String match_cloud_filename){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetCloudFileNameFilter(handle, match_cloud_filename.utf8().get_data());
}

// Sets the folder that will be stored as the content for an item.
bool SteamServer::setItemContent(uint64_t update_handle, String content_folder){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemContent(handle, content_folder.utf8().get_data());
}

// Sets a new description for an item.
bool SteamServer::setItemDescription(uint64_t update_handle, String description){
	if(SteamUGC() == NULL){
		return false;
	}
	if ((uint32_t)description.length() > (uint32_t)k_cchPublishedDocumentDescriptionMax){
		printf("Description cannot have more than %d ASCII characters. Description not set.", k_cchPublishedDocumentDescriptionMax);
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemDescription(handle, description.utf8().get_data());
}

// Sets arbitrary metadata for an item. This metadata can be returned from queries without having to download and install the actual content.
bool SteamServer::setItemMetadata(uint64_t update_handle, String metadata){
	if(SteamUGC() == NULL){
		return false;
	}
	if (metadata.utf8().length() > 5000){
		printf("Metadata cannot be more than %d bytes. Metadata not set.", 5000);
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemMetadata(handle, metadata.utf8().get_data());
}

// Sets the primary preview image for the item.
bool SteamServer::setItemPreview(uint64_t update_handle, String preview_file){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemPreview(handle, preview_file.utf8().get_data());
}

// Sets arbitrary developer specified tags on an item.
bool SteamServer::setItemTags(uint64_t update_handle, Array tag_array, bool allow_admin_tags){
	bool tags_set = false;
	if(SteamUGC() != NULL){
		UGCUpdateHandle_t handle = uint64(update_handle);
		SteamParamStringArray_t *tags = new SteamParamStringArray_t();
		tags->m_ppStrings = new const char*[tag_array.size()];
		uint32 strCount = tag_array.size();
		for (uint32 i=0; i < strCount; i++) {
			String str = tag_array[i];
			tags->m_ppStrings[i] = str.utf8().get_data();
		}
		tags->m_nNumStrings = tag_array.size();
		tags_set = SteamUGC()->SetItemTags(handle, tags, allow_admin_tags);
		delete tags;
	}
	return tags_set;
}

// Sets a new title for an item.
bool SteamServer::setItemTitle(uint64_t update_handle, String title){
	if(SteamUGC() == NULL){
		return false;
	}
	if (title.length() > 255){
		printf("Title cannot have more than %d ASCII characters. Title not set.", 255);
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemTitle(handle, title.utf8().get_data());
}

// Sets the language of the title and description that will be set in this item update.
bool SteamServer::setItemUpdateLanguage(uint64_t update_handle, String language){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemUpdateLanguage(handle, language.utf8().get_data());
}

// Sets the visibility of an item.
bool SteamServer::setItemVisibility(uint64_t update_handle, int visibility){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetItemVisibility(handle, (ERemoteStoragePublishedFileVisibility)visibility);
}

// Sets the language to return the title and description in for the items on a pending UGC Query.
bool SteamServer::setLanguage(uint64_t query_handle, String language){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetLanguage(handle, language.utf8().get_data());
}

// Sets whether workshop items will be returned if they have one or more matching tag, or if all tags need to match on a pending UGC Query.
bool SteamServer::setMatchAnyTag(uint64_t query_handle, bool match_any_tag){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetMatchAnyTag(handle, match_any_tag);
}

// Sets whether the order of the results will be updated based on the rank of items over a number of days on a pending UGC Query.
bool SteamServer::setRankedByTrendDays(uint64_t query_handle, uint32 days){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetRankedByTrendDays(handle, days);
}

// Sets whether to return any additional images/videos attached to the items on a pending UGC Query.
bool SteamServer::setReturnAdditionalPreviews(uint64_t query_handle, bool return_additional_previews){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnAdditionalPreviews(handle, return_additional_previews);
}

// Sets whether to return the IDs of the child items of the items on a pending UGC Query.
bool SteamServer::setReturnChildren(uint64_t query_handle, bool return_children){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnChildren(handle, return_children);
}

// Sets whether to return any key-value tags for the items on a pending UGC Query.
bool SteamServer::setReturnKeyValueTags(uint64_t query_handle, bool return_key_value_tags){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnKeyValueTags(handle, return_key_value_tags);
}

// Sets whether to return the full description for the items on a pending UGC Query.
bool SteamServer::setReturnLongDescription(uint64_t query_handle, bool return_long_description){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnLongDescription(handle, return_long_description);
}

// Sets whether to return the developer specified metadata for the items on a pending UGC Query.
bool SteamServer::setReturnMetadata(uint64_t query_handle, bool return_metadata){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnMetadata(handle, return_metadata);
}

// Sets whether to only return IDs instead of all the details on a pending UGC Query.
bool SteamServer::setReturnOnlyIDs(uint64_t query_handle, bool return_only_ids){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnOnlyIDs(handle, return_only_ids);
}

// Sets whether to return the the playtime stats on a pending UGC Query.
bool SteamServer::setReturnPlaytimeStats(uint64_t query_handle, uint32 days){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnPlaytimeStats(handle, days);
}

// Sets whether to only return the the total number of matching items on a pending UGC Query.
bool SteamServer::setReturnTotalOnly(uint64_t query_handle, bool return_total_only){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetReturnTotalOnly(handle, return_total_only);
}

// Sets a string to that items need to match in either the title or the description on a pending UGC Query.
bool SteamServer::setSearchText(uint64_t query_handle, String search_text){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCQueryHandle_t handle = (uint64_t)query_handle;
	return SteamUGC()->SetSearchText(handle, search_text.utf8().get_data());
}

// Allows the user to rate a workshop item up or down.
void SteamServer::setUserItemVote(uint64_t published_file_id, bool vote_up){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->SetUserItemVote(file_id, vote_up);
		callResultSetUserItemVote.Set(api_call, this, &SteamServer::set_user_item_vote);
	}
}

// Starts the item update process.
uint64_t SteamServer::startItemUpdate(uint32_t app_id, uint64_t published_file_id){
	if(SteamUGC() == NULL){
		return 0;
	}
	AppId_t app = (uint32_t)app_id;
	PublishedFileId_t file_id = (uint64_t)published_file_id;
	return SteamUGC()->StartItemUpdate(app, file_id);
}

// Start tracking playtime on a set of workshop items.
void SteamServer::startPlaytimeTracking(Array published_file_ids){
	if(SteamUGC() != NULL){
		uint32 fileCount = published_file_ids.size();
		if(fileCount > 0){
			PublishedFileId_t *file_ids = new PublishedFileId_t[fileCount];
			for(uint32 i = 0; i < fileCount; i++){
				file_ids[i] = (uint64_t)published_file_ids[i];
			}
			SteamAPICall_t api_call = SteamUGC()->StartPlaytimeTracking(file_ids, fileCount);
			callResultStartPlaytimeTracking.Set(api_call, this, &SteamServer::start_playtime_tracking);
			delete[] file_ids;
		}
	}
}

// Stop tracking playtime on a set of workshop items.
void SteamServer::stopPlaytimeTracking(Array published_file_ids){
	if(SteamUGC() != NULL){
		uint32 fileCount = published_file_ids.size();
		if(fileCount > 0){
			PublishedFileId_t *file_ids = new PublishedFileId_t[fileCount];
			Array files;
			for(uint32 i = 0; i < fileCount; i++){
				file_ids[i] = (uint64_t)published_file_ids[i];
			}
			SteamAPICall_t api_call = SteamUGC()->StopPlaytimeTracking(file_ids, fileCount);
			callResultStopPlaytimeTracking.Set(api_call, this, &SteamServer::stop_playtime_tracking);
			delete[] file_ids;
		}
	}
}

// Stop tracking playtime of all workshop items.
void SteamServer::stopPlaytimeTrackingForAllItems(){
	if(SteamUGC() != NULL){
		SteamAPICall_t api_call = SteamUGC()->StopPlaytimeTrackingForAllItems();
		callResultStopPlaytimeTracking.Set(api_call, this, &SteamServer::stop_playtime_tracking);
	}
}

// Returns any app dependencies that are associated with the given item.
void SteamServer::getAppDependencies(uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->GetAppDependencies(file_id);
		callResultGetAppDependencies.Set(api_call, this, &SteamServer::get_app_dependencies_result);
	}
}

// Uploads the changes made to an item to the Steam Workshop; to be called after setting your changes.
void SteamServer::submitItemUpdate(uint64_t update_handle, String change_note){
	if(SteamUGC() != NULL){
		UGCUpdateHandle_t handle = uint64(update_handle);
		SteamAPICall_t api_call;
		if (change_note.empty()) {
			api_call = SteamUGC()->SubmitItemUpdate(handle, NULL);
		} else {
			api_call = SteamUGC()->SubmitItemUpdate(handle, change_note.utf8().get_data());
		}
		callResultItemUpdate.Set(api_call, this, &SteamServer::item_updated);
	}
}

// Subscribe to a workshop item. It will be downloaded and installed as soon as possible.
void SteamServer::subscribeItem(uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->SubscribeItem(file_id);
		callResultSubscribeItem.Set(api_call, this, &SteamServer::subscribe_item);
	}
}

// SuspendDownloads( true ) will suspend all workshop downloads until SuspendDownloads( false ) is called or the game ends.
void SteamServer::suspendDownloads(bool suspend){
	if(SteamUGC() != NULL){
		SteamUGC()->SuspendDownloads(suspend);
	}
}

// Unsubscribe from a workshop item. This will result in the item being removed after the game quits.
void SteamServer::unsubscribeItem(uint64_t published_file_id){
	if(SteamUGC() != NULL){
		PublishedFileId_t file_id = (uint64_t)published_file_id;
		SteamAPICall_t api_call = SteamUGC()->UnsubscribeItem(file_id);
		callResultUnsubscribeItem.Set(api_call, this, &SteamServer::unsubscribe_item);
	}
}

// Updates an existing additional preview file for the item.
bool SteamServer::updateItemPreviewFile(uint64_t update_handle, uint32 index, String preview_file){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->UpdateItemPreviewFile(handle, index, preview_file.utf8().get_data());
}

// Updates an additional video preview from YouTube for the item.
bool SteamServer::updateItemPreviewVideo(uint64_t update_handle, uint32 index, String video_id){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->UpdateItemPreviewVideo(handle, index, video_id.utf8().get_data());
}

// Show the app's latest Workshop EULA to the user in an overlay window, where they can accept it or not.
bool SteamServer::showWorkshopEULA(){
	if(SteamUGC() == NULL){
		return false;
	}
	return SteamUGC()->ShowWorkshopEULA();
}

// Retrieve information related to the user's acceptance or not of the app's specific Workshop EULA.
void SteamServer::getWorkshopEULAStatus(){
	if(SteamUGC() != NULL){
		SteamAPICall_t api_call = SteamUGC()->GetWorkshopEULAStatus();
		callResultWorkshopEULAStatus.Set(api_call, this, &SteamServer::workshop_eula_status);
	}
}

// Set the time range this item was created.
bool SteamServer::setTimeCreatedDateRange(uint64_t update_handle, uint32 start, uint32 end){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetTimeCreatedDateRange(handle, start, end);
}

// Set the time range this item was updated.
bool SteamServer::setTimeUpdatedDateRange(uint64_t update_handle, uint32 start, uint32 end){
	if(SteamUGC() == NULL){
		return false;
	}
	UGCUpdateHandle_t handle = uint64(update_handle);
	return SteamUGC()->SetTimeUpdatedDateRange(handle, start, end);
}


/////////////////////////////////////////////////
///// SIGNALS / CALLBACKS
/////////////////////////////////////////////////
//
// GAME SERVER CALLBACKS ////////////////////////
//
// Called when a connection attempt has failed. This will occur periodically if the Steam client is not connected, and has failed when retrying to establish a connection.
void SteamServer::server_connect_failure(SteamServerConnectFailure_t* server_data){
	int result = server_data->m_eResult;
	bool retrying = server_data->m_bStillRetrying;
	emit_signal("server_connect_failure", result, retrying);
}

// Server has connected to the Steam back-end; server_data has no fields.
void SteamServer::server_connected(SteamServersConnected_t* server_data){
	emit_signal("server_connected");
}

// Called if the client has lost connection to the Steam servers. Real-time services will be disabled until a matching SteamServersConnected_t has been posted.
void SteamServer::server_disconnected(SteamServersDisconnected_t* server_data){
	int result = server_data->m_eResult;
	emit_signal("server_disconnected", result);
}

// Client has been approved to connect to this game server.
void SteamServer::client_approved(GSClientApprove_t* client_data){
	uint64_t steam_id = client_data->m_SteamID.ConvertToUint64();
	uint64_t owner_id = client_data->m_OwnerSteamID.ConvertToUint64();
	emit_signal("client_approved", steam_id, owner_id);
}

// Client has been denied to connection to this game server.
void SteamServer::client_denied(GSClientDeny_t* client_data){
	uint64_t steam_id = client_data->m_SteamID.ConvertToUint64();
	int reason = client_data->m_eDenyReason;
	emit_signal("client_denied", steam_id, reason);
}

// Request the game server should kick the user.
void SteamServer::client_kick(GSClientKick_t* client_data){
	uint64_t steam_id = client_data->m_SteamID.ConvertToUint64();
	int reason = client_data->m_eDenyReason;
	emit_signal("client_kick", steam_id, reason);
}

// Received when the game server requests to be displayed as secure (VAC protected).
// m_bSecure is true if the game server should display itself as secure to users, false otherwise.
void SteamServer::policy_response(GSPolicyResponse_t* policyData){
	uint8 secure = policyData->m_bSecure;
	emit_signal("policy_response", secure);
}

// Sent as a reply to RequestUserGroupStatus().
void SteamServer::client_group_status(GSClientGroupStatus_t* client_data){
	uint64_t steam_id = client_data->m_SteamIDUser.ConvertToUint64();
	uint64_t group_id = client_data->m_SteamIDGroup.ConvertToUint64();
	bool member = client_data->m_bMember;
	bool officer = client_data->m_bOfficer;
	emit_signal("client_group_status", steam_id, group_id, member, officer);
}

// Sent as a reply to AssociateWithClan().
void SteamServer::associate_clan(AssociateWithClanResult_t* clanData){
	int result = clanData->m_eResult;
	emit_signal("associate_clan", result);
}

// Sent as a reply to ComputeNewPlayerCompatibility().
void SteamServer::player_compat(ComputeNewPlayerCompatibilityResult_t* player_data){
	int result = player_data->m_eResult;
	int players_dont_like_candidate = player_data->m_cPlayersThatDontLikeCandidate;
	int players_candidate_doesnt_like = player_data->m_cPlayersThatCandidateDoesntLike;
	int clan_players_dont_like_candidate = player_data->m_cClanPlayersThatDontLikeCandidate;
	uint64_t steam_id = player_data->m_SteamIDCandidate.ConvertToUint64();
	emit_signal("player_compat", result, players_dont_like_candidate, players_candidate_doesnt_like, clan_players_dont_like_candidate, steam_id);
}

// GAME SERVER STATS CALLBACKS //////////////////
//
// Result when getting the latests stats and achievements for a user from the server.
void SteamServer::stats_received(GSStatsReceived_t* callData, bool bioFailure){
	EResult result = callData->m_eResult;
	uint64_t steam_id = callData->m_steamIDUser.ConvertToUint64();
	emit_signal("stats_received", result, steam_id);
}

// Result of a request to store the user stats.
void SteamServer::stats_stored(GSStatsStored_t* callData){
	EResult result = callData->m_eResult;
	uint64_t steam_id = callData->m_steamIDUser.ConvertToUint64();
	emit_signal("stats_stored", result, steam_id);
}

// Callback indicating that a user's stats have been unloaded.
void SteamServer::stats_unloaded(GSStatsUnloaded_t* callData){
	uint64_t steam_id = callData->m_steamIDUser.ConvertToUint64();
	emit_signal("stats_unloaded", steam_id);
}

// HTTP CALLBACKS ///////////////////////////////
//
//! Result when an HTTP request completes. If you're using GetHTTPStreamingResponseBodyData then you should be using the HTTPRequestHeadersReceived_t or HTTPRequestDataReceived_t.
void SteamServer::http_request_completed(HTTPRequestCompleted_t* call_data){
	uint32 cookie_handle = call_data->m_hRequest;
	uint64_t context_value = call_data->m_ulContextValue;
	bool request_success = call_data->m_bRequestSuccessful;
	int status_code = call_data->m_eStatusCode;
	uint32 body_size = call_data->m_unBodySize;
	emit_signal("http_request_completed", cookie_handle, context_value, request_success, status_code, body_size);
}

//! Triggered when a chunk of data is received from a streaming HTTP request.
void SteamServer::http_request_data_received(HTTPRequestDataReceived_t* call_data){
	uint32 cookie_handle = call_data->m_hRequest;
	uint64_t context_value = call_data->m_ulContextValue;
	uint32 offset = call_data->m_cOffset;
	uint32 bytes_received = call_data->m_cBytesReceived;
	emit_signal("http_request_data_received", cookie_handle, context_value, offset, bytes_received);
}

//! Triggered when HTTP headers are received from a streaming HTTP request.
void SteamServer::http_request_headers_received(HTTPRequestHeadersReceived_t* call_data){
	uint32 cookie_handle = call_data->m_hRequest;
	uint64_t context_value = call_data->m_ulContextValue;
	emit_signal("http_request_headers_received", cookie_handle, context_value);
}

// INVENTORY CALLBACKS //////////////////////////
//
// This callback is triggered whenever item definitions have been updated, which could be in response to LoadItemDefinitions or any time new item definitions are available (eg, from the dynamic addition of new item types while players are still in-game).
void SteamServer::inventory_definition_update(SteamInventoryDefinitionUpdate_t *call_data){
	// Create the return array
	Array definitions;
	// Set the array size variable
	uint32 size = 0;
	// Get the item defition IDs
	if(SteamInventory()->GetItemDefinitionIDs(NULL, &size)){
		SteamItemDef_t *id_array = new SteamItemDef_t[size];
		if(SteamInventory()->GetItemDefinitionIDs(id_array, &size)){
			// Loop through the temporary array and populate the return array
			for(uint32 i = 0; i < size; i++){
				definitions.append(id_array[i]);
			}
		}
		// Delete the temporary array
		delete[] id_array;
	}
	// Return the item array as a signal
	emit_signal("inventory_defintion_update", definitions);
}

// Triggered when GetAllItems successfully returns a result which is newer / fresher than the last known result. (It will not trigger if the inventory hasn't changed, or if results from two overlapping calls are reversed in flight and the earlier result is already known to be stale/out-of-date.)
// The regular SteamInventoryResultReady_t callback will still be triggered immediately afterwards; this is an additional notification for your convenience.
void SteamServer::inventory_full_update(SteamInventoryFullUpdate_t *call_data){
	// Set the handle
	inventory_handle = call_data->m_handle;
	// Send the handle back to the user
	emit_signal("inventory_full_update", call_data->m_handle);
}

// This is fired whenever an inventory result transitions from k_EResultPending to any other completed state, see GetResultStatus for the complete list of states. There will always be exactly one callback per handle.
void SteamServer::inventory_result_ready(SteamInventoryResultReady_t *call_data){
	// Get the result
	int result = call_data->m_result;
	// Get the handle and pass it over
	inventory_handle = call_data->m_handle;
	emit_signal("inventory_result_ready", result, inventory_handle);
}

// NETWORKING CALLBACKS /////////////////////////
//
// Called when packets can't get through to the specified user. All queued packets unsent at this point will be dropped, further attempts to send will retry making the connection (but will be dropped if we fail again).
void SteamServer::p2p_session_connect_fail(P2PSessionConnectFail_t* call_data) {
	uint64_t remote_steam_id = call_data->m_steamIDRemote.ConvertToUint64();
	uint8_t session_error = call_data->m_eP2PSessionError;
	emit_signal("p2p_session_connect_fail", remote_steam_id, session_error);
}

// A user wants to communicate with us over the P2P channel via the sendP2PPacket. In response, a call to acceptP2PSessionWithUser needs to be made, if you want to open the network channel with them.
void SteamServer::p2p_session_request(P2PSessionRequest_t* call_data){
	uint64_t remote_steam_id = call_data->m_steamIDRemote.ConvertToUint64();
	emit_signal("p2p_session_request", remote_steam_id);
}

// NETWORKING MESSAGES CALLBACKS ////////////////
//
// Posted when a remote host is sending us a message, and we do not already have a session with them.
void SteamServer::network_messages_session_request(SteamNetworkingMessagesSessionRequest_t* call_data){
	emit_signal("network_messages_session_request", getSteamIDFromIdentity(call_data->m_identityRemote));
}

// Posted when we fail to establish a connection, or we detect that communications have been disrupted it an unusual way.
void SteamServer::network_messages_session_failed(SteamNetworkingMessagesSessionFailed_t* call_data){
	SteamNetConnectionInfo_t info = call_data->m_info;
	// Parse out the reason for failure
	int reason = info.m_eEndReason;
	emit_signal("network_messages_session_failed", reason);
}

// NETWORKING SOCKETS CALLBACKS /////////////////
//
// This callback is posted whenever a connection is created, destroyed, or changes state. The m_info field will contain a complete description of the connection at the time the change occurred and the callback was posted. In particular, m_info.m_eState will have the new connection state.
void SteamServer::network_connection_status_changed(SteamNetConnectionStatusChangedCallback_t* call_data){
	// Connection handle.
	uint64_t connect_handle = call_data->m_hConn;
	// Full connection info.
	SteamNetConnectionInfo_t connection_info = call_data->m_info;
	// Move connection info into a dictionary
	Dictionary connection;
	connection["identity"] = getSteamIDFromIdentity(connection_info.m_identityRemote);
	connection["user_data"] = (uint64_t)connection_info.m_nUserData;
	connection["listen_socket"] = connection_info.m_hListenSocket;
	connection["remote_address"] = getStringFromSteamIP(connection_info.m_addrRemote);
	connection["remote_pop"] = connection_info.m_idPOPRemote;
	connection["pop_relay"] = connection_info.m_idPOPRelay;
	connection["connection_state"] = connection_info.m_eState;
	connection["end_reason"] = connection_info.m_eEndReason;
	connection["end_debug"] = connection_info.m_szEndDebug;
	connection["debug_description"] = connection_info.m_szConnectionDescription;
	// Previous state (current state is in m_info.m_eState).
	int old_state = call_data->m_eOldState;
	// Send the data back via signal
	emit_signal("network_connection_status_changed", connect_handle, connection, old_state);
}

// This callback is posted whenever the state of our readiness changes.
void SteamServer::network_authentication_status(SteamNetAuthenticationStatus_t* call_data){
	// Status.
	int available = call_data->m_eAvail;
	// Non-localized English language status. For diagnostic / debugging purposes only.
	char *debug_message = new char[256];
	snprintf(debug_message, 256, "%s", call_data->m_debugMsg);
	// Send the data back via signal
	emit_signal("network_authentication_status", available, debug_message);
	delete[] debug_message;
}

// A struct used to describe a "fake IP" we have been assigned to use as an identifier.
// This callback is posted when ISteamNetworkingSoockets::BeginAsyncRequestFakeIP completes.
void SteamServer::fake_ip_result(SteamNetworkingFakeIPResult_t* call_data){
	int result = call_data->m_eResult;
	uint32 fake_ip = call_data->m_unIP;
	// Get the ports as an array
	Array port_list;
	uint16* ports = call_data->m_unPorts;
	for(uint16 i = 0; i < sizeof(ports); i++){
		port_list.append(ports[i]);
	}
	emit_signal("fake_ip_result", result, getSteamIDFromIdentity(call_data->m_identity), getStringFromIP(fake_ip), port_list);
}

// NETWORKING UTILS CALLBACKS ///////////////////
//
// A struct used to describe our readiness to use the relay network.
void SteamServer::relay_network_status(SteamRelayNetworkStatus_t* call_data){
	int available = call_data->m_eAvail;
	int ping_measurement = call_data->m_bPingMeasurementInProgress;
	int available_config = call_data->m_eAvailNetworkConfig;
	int available_relay = call_data->m_eAvailAnyRelay;
	char *debug_message = new char[256];
	sprintf(debug_message, "%s", call_data->m_debugMsg);
//	debug_message = call_data->m_debugMsg;
	emit_signal("relay_network_status", available, ping_measurement, available_config, available_relay, debug_message);
	delete[] debug_message;
}

// REMOTE STORAGE CALLBACKS /////////////////////
//
// Purpose: one or more files for this app have changed locally after syncing to remote session changes.
// Note: only posted if this happens DURING the local app session.
void SteamServer::local_file_changed(RemoteStorageLocalFileChange_t* call_data){
	emit_signal("local_file_changed");
}

// UGC CALLBACKS ////////////////////////////////
//
// Called when a workshop item has been downloaded.
void SteamServer::item_downloaded(DownloadItemResult_t* call_data){
	EResult result = call_data->m_eResult;
	PublishedFileId_t file_id = call_data->m_nPublishedFileId;
	AppId_t app_id = call_data->m_unAppID;
	emit_signal("item_downloaded", result, (uint64_t)file_id, (uint32_t)app_id);
}

// Called when a workshop item has been installed or updated.
void SteamServer::item_installed(ItemInstalled_t* call_data){
	AppId_t app_id = call_data->m_unAppID;
	PublishedFileId_t file_id = call_data->m_nPublishedFileId;
	emit_signal("item_installed", app_id, (uint64_t)file_id);
}

// Purpose: signal that the list of subscribed items changed.
void SteamServer::user_subscribed_items_list_changed(UserSubscribedItemsListChanged_t* call_data){
	uint32 app_id = call_data->m_nAppID;
	emit_signal("user_subscribed_items_list_changed", app_id);
}


///// SIGNALS / CALL RESULTS
/////////////////////////////////////////////////
//
// STEAMWORKS ERROR SIGNAL //////////////////////
//
//! Intended to serve as generic error messaging for failed call results
void SteamServer::steamworksError(String failed_signal){
	// Emit the signal to inform the user of the failure
	emit_signal("steamworks_error", failed_signal, "io failure");
}

// INVENTORY CALL RESULTS ///////////////////////
//
// Returned when you have requested the list of "eligible" promo items that can be manually granted to the given user. These are promo items of type "manual" that won't be granted automatically.
void SteamServer::inventory_eligible_promo_item(SteamInventoryEligiblePromoItemDefIDs_t *call_data, bool io_failure){
	if(io_failure){
		steamworksError("inventory_eligible_promo_item");
	}
	else{
		// Clean up call data
		CSteamID steam_id = call_data->m_steamID;
		int result = call_data->m_result;
		int eligible = call_data->m_numEligiblePromoItemDefs;
		bool cached = call_data->m_bCachedData;
		// Create the return array
		Array definitions;
		// Create the temporary ID array
		SteamItemDef_t *id_array = new SteamItemDef_t[eligible];
		// Convert eligible size
		uint32 array_size = (int)eligible;
		// Get the list
		if(SteamInventory()->GetEligiblePromoItemDefinitionIDs(steam_id, id_array, &array_size)){
			// Loop through the temporary array and populate the return array
			for(int i = 0; i < eligible; i++){
				definitions.append(id_array[i]);
			}
		}
		// Delete the temporary array
		delete[] id_array;
		// Return the item array as a signal
		emit_signal("inventory_eligible_promo_Item", result, cached, definitions);
	}
}

// Returned after StartPurchase is called.
void SteamServer::inventory_start_purchase_result(SteamInventoryStartPurchaseResult_t *call_data, bool io_failure){
	if(io_failure){
		steamworksError("inventory_start_purchase_result");
	}
	else{
		if(call_data->m_result == k_EResultOK){
			uint64_t order_id = call_data->m_ulOrderID;
			uint64_t transaction_id = call_data->m_ulTransID;
			emit_signal("inventory_start_purchase_result", "success", order_id, transaction_id);
		}
		else{
			emit_signal("inventory_start_purchase_result", "failure", 0, 0);
		}
	}
}

// Returned after RequestPrices is called.
void SteamServer::inventory_request_prices_result(SteamInventoryRequestPricesResult_t *call_data, bool io_failure){
	if(io_failure){
		steamworksError("inventory_request_prices_result");
	}
	else{
		int result = call_data->m_result;
		String currency = call_data->m_rgchCurrency;
		emit_signal("inventory_request_prices_result", result, currency);
	}
}

// REMOTE STORAGE CALL RESULTS //////////////////
//
// Response when reading a file asyncrounously with FileReadAsync.
void SteamServer::file_read_async_complete(RemoteStorageFileReadAsyncComplete_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("file_read_async_complete");
	}
	else{
		uint64_t handle = call_data->m_hFileReadAsync;
		int result = call_data->m_eResult;
		uint32 offset = call_data->m_nOffset;
		uint32 read = call_data->m_cubRead;
		// Was read complete?
		PoolByteArray buffer;
		buffer.resize(read);
		bool complete = SteamRemoteStorage()->FileReadAsyncComplete(handle, buffer.write().ptr(), read);
		// Create a dictionary and populate it with the results
		Dictionary file_read;
		file_read["result"] = result;
		file_read["handle"] = handle;
		file_read["buffer"] = buffer;
		file_read["offset"] = offset;
		file_read["read"] = read;
		file_read["complete"] = complete;
		emit_signal("file_read_async_complete", file_read);
	}
}

// Response to a file being shared.
void SteamServer::file_share_result(RemoteStorageFileShareResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("file_share_result");
	}
	else{
		int result = call_data->m_eResult;
		uint64_t handle = call_data->m_hFile;
		char name[k_cchFilenameMax];
		strcpy(name, call_data->m_rgchFilename);
		emit_signal("file_share_result", result, handle, name);
	}
}

// Response when writing a file asyncrounously with FileWriteAsync.
void SteamServer::file_write_async_complete(RemoteStorageFileWriteAsyncComplete_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("file_write_async_complete");
	}
	else{
		int result = call_data->m_eResult;
		emit_signal("file_write_async_complete", result);
	}
}

// Response when downloading UGC
void SteamServer::download_ugc_result(RemoteStorageDownloadUGCResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("download_ugc_result");
	}
	else{
		int result = call_data->m_eResult;
		uint64_t handle = call_data->m_hFile;
		uint32_t app_id = call_data->m_nAppID;
		int32 size = call_data->m_nSizeInBytes;
		char filename[k_cchFilenameMax];
		strcpy(filename, call_data->m_pchFileName);
		uint64_t owner_id = call_data->m_ulSteamIDOwner;
		// Pass some variable to download dictionary to bypass argument limit
		Dictionary download_data;
		download_data["handle"] = handle;
		download_data["app_id"] = app_id;
		download_data["size"] = size;
		download_data["filename"] = filename;
		download_data["owner_id"] = owner_id;
		emit_signal("download_ugc_result", result, download_data);
	}
}

// Called when the user has unsubscribed from a piece of UGC. Result from ISteamUGC::UnsubscribeItem.
void SteamServer::unsubscribe_item(RemoteStorageUnsubscribePublishedFileResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("unsubscribe_item");
	}
	else{
		int result = call_data->m_eResult;
		int file_id = call_data->m_nPublishedFileId;
		emit_signal("unsubscribe_item", result, file_id);
	}
}

// Called when the user has subscribed to a piece of UGC. Result from ISteamUGC::SubscribeItem.
void SteamServer::subscribe_item(RemoteStorageSubscribePublishedFileResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("subscribe_item");
	}
	else{
		int result = call_data->m_eResult;
		int file_id = call_data->m_nPublishedFileId;
		emit_signal("subscribe_item", result, file_id);
	}
}

// UGC CALL RESULTS /////////////////////////////
//
// The result of a call to AddAppDependency.
void SteamServer::add_app_dependency_result(AddAppDependencyResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("add_app_dependency_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		AppId_t app_id = call_data->m_nAppID;
		emit_signal("add_app_dependency_result", result, (uint64_t)file_id, (uint32_t)app_id);
	}
}

// The result of a call to AddDependency.
void SteamServer::add_ugc_dependency_result(AddUGCDependencyResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("add_ugc_dependency_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		PublishedFileId_t child_id = call_data->m_nChildPublishedFileId;
		emit_signal("add_ugc_dependency_result", result, (uint64_t)file_id, (uint64_t)child_id);
	}
}

// Result of a workshop item being created.
void SteamServer::item_created(CreateItemResult_t *call_data, bool io_failure){
	if(io_failure){
		steamworksError("item_created");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		bool accept_tos = call_data->m_bUserNeedsToAcceptWorkshopLegalAgreement;
		emit_signal("item_created", result, (uint64_t)file_id, accept_tos);
	}
}

// Called when getting the app dependencies for an item.
void SteamServer::get_app_dependencies_result(GetAppDependenciesResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("get_app_dependencies_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
//		AppId_t app_id = call_data->m_rgAppIDs;
		uint32 app_dependencies = call_data->m_nNumAppDependencies;
		uint32 total_app_dependencies = call_data->m_nTotalNumAppDependencies;
//		emit_signal("get_app_dependencies_result", result, (uint64_t)file_id, app_id, appDependencies, totalAppDependencies);
		emit_signal("get_app_dependencies_result", result, (uint64_t)file_id, app_dependencies, total_app_dependencies);
	}
}

// Called when an attempt at deleting an item completes.
void SteamServer::item_deleted(DeleteItemResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("item_deleted");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		emit_signal("item_deleted", result, (uint64_t)file_id);
	}
}

// Called when getting the users vote status on an item.
void SteamServer::get_item_vote_result(GetUserItemVoteResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("get_item_vote_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		bool vote_up = call_data->m_bVotedUp;
		bool vote_down = call_data->m_bVotedDown;
		bool vote_skipped = call_data->m_bVoteSkipped;
		emit_signal("get_item_vote_result", result, (uint64_t)file_id, vote_up, vote_down, vote_skipped);
	}
}

// Purpose: The result of a call to RemoveAppDependency.
void SteamServer::remove_app_dependency_result(RemoveAppDependencyResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("remove_app_dependency_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		AppId_t app_id = call_data->m_nAppID;
		emit_signal("remove_app_dependency_result", result, (uint64_t)file_id, (uint32_t)app_id);
	}
}

// Purpose: The result of a call to RemoveDependency.
void SteamServer::remove_ugc_dependency_result(RemoveUGCDependencyResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("remove_ugc_dependency_result");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		PublishedFileId_t child_id = call_data->m_nChildPublishedFileId;
		emit_signal("remove_ugc_dependency_result", result, (uint64_t)file_id, (uint64_t)child_id);
	}
}

// Called when the user has voted on an item.
void SteamServer::set_user_item_vote(SetUserItemVoteResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("set_user_item_vote");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		bool vote_up = call_data->m_bVoteUp;
		emit_signal("set_user_item_vote", result, (uint64_t)file_id, vote_up);
	}
}

// Called when workshop item playtime tracking has started.
void SteamServer::start_playtime_tracking(StartPlaytimeTrackingResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("start_playtime_tracking");
	}
	else{
		EResult result = call_data->m_eResult;
		emit_signal("start_playtime_tracking", result);
	}
}

// Called when a UGC query request completes.
void SteamServer::ugc_query_completed(SteamUGCQueryCompleted_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("ugc_query_completed");
	}
	else{
		UGCQueryHandle_t handle = call_data->m_handle;
		EResult result = call_data->m_eResult;
		uint32 results_returned = call_data->m_unNumResultsReturned;
		uint32 total_matching = call_data->m_unTotalMatchingResults;
		bool cached = call_data->m_bCachedData;
		emit_signal("ugc_query_completed", (uint64_t)handle, result, results_returned, total_matching, cached);
	}
}

// Called when workshop item playtime tracking has stopped.
void SteamServer::stop_playtime_tracking(StopPlaytimeTrackingResult_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("stop_playtime_tracking");
	}
	else{
		EResult result = call_data->m_eResult;
		emit_signal("stop_playtime_tracking", result);
	}
}

// Result of a workshop item being updated.
void SteamServer::item_updated(SubmitItemUpdateResult_t *call_data, bool io_failure){
	if(io_failure){
		steamworksError("item_updated");
	}
	else{
		EResult result = call_data->m_eResult;
		bool need_to_accept_tos = call_data->m_bUserNeedsToAcceptWorkshopLegalAgreement;
		emit_signal("item_updated", result, need_to_accept_tos);
	}
}

// Called when the user has added or removed an item to/from their favorites.
void SteamServer::user_favorite_items_list_changed(UserFavoriteItemsListChanged_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("user_favorite_items_list_changed");
	}
	else{
		EResult result = call_data->m_eResult;
		PublishedFileId_t file_id = call_data->m_nPublishedFileId;
		bool was_add_request = call_data->m_bWasAddRequest;
		emit_signal("user_favorite_items_list_changed", result, (uint64_t)file_id, was_add_request);
	}
}

// Purpose: Status of the user's acceptable/rejection of the app's specific Workshop EULA.
void SteamServer::workshop_eula_status(WorkshopEULAStatus_t* call_data, bool io_failure){
	if(io_failure){
		steamworksError("workshop_eula_status");
	}
	else{
		int result = call_data->m_eResult;
		uint32 app_id = call_data->m_nAppID;
		// Slim down signal arguments since Godot seems to limit them to six max
		Dictionary eula_data;
		eula_data["version"] = call_data->m_unVersion;			// int
		eula_data["action"] = call_data->m_rtAction;			// int
		eula_data["accepted"] = call_data->m_bAccepted;			// bool
		eula_data["needs_action"] = call_data->m_bNeedsAction;	// bool
		emit_signal("workshop_eula_status", result, app_id, eula_data);
	}
}


///// BIND METHODS
/////////////////////////////////////////////////
//
void SteamServer::_register_methods(){

	// FUNCTION BINDS
	/////////////////////////////////////////////
	//
	// MAIN BIND METHODS ////////////////////////
	register_method("getServerSteamID", &SteamServer::getServerSteamID);
	register_method("getSteamID32", &SteamServer::getSteamID32);
	register_method("isAnonAccount", &SteamServer::isAnonAccount);
	register_method("isAnonUserAccount", &SteamServer::isAnonUserAccount);
	register_method("isChatAccount", &SteamServer::isChatAccount);
	register_method("isClanAccount", &SteamServer::isClanAccount);
	register_method("isConsoleUserAccount", &SteamServer::isConsoleUserAccount);
	register_method("isIndividualAccount", &SteamServer::isIndividualAccount);
	register_method("isLobby", &SteamServer::isLobby);
	register_method("isServerSecure", &SteamServer::isServerSecure);
	register_method("run_callbacks", &SteamServer::run_callbacks);
	register_method("serverInit", &SteamServer::serverInit);
	register_method("serverInitEx", &SteamServer::serverInitEx);
	register_method("serverReleaseCurrentThreadMemory", &SteamServer::serverReleaseCurrentThreadMemory);
	register_method("serverShutdown", &SteamServer::serverShutdown);

	// GAME SERVER BIND METHODS /////////////////
	register_method("associateWithClan", &SteamServer::associateWithClan);
	register_method("beginAuthSession", &SteamServer::beginAuthSession);
	register_method("cancelAuthTicket", &SteamServer::cancelAuthTicket);
	register_method("clearAllKeyValues", &SteamServer::clearAllKeyValues);
	register_method("computeNewPlayerCompatibility", &SteamServer::computeNewPlayerCompatibility);
	register_method("endAuthSession", &SteamServer::endAuthSession);
	register_method("getAuthSessionTicket", &SteamServer::getAuthSessionTicket);
	register_method("getNextOutgoingPacket", &SteamServer::getNextOutgoingPacket);
	register_method("getPublicIP", &SteamServer::getPublicIP);
	register_method("getSteamID", &SteamServer::getSteamID);
	register_method("handleIncomingPacket", &SteamServer::handleIncomingPacket);
	register_method("loggedOn", &SteamServer::loggedOn);
	register_method("logOff", &SteamServer::logOff);
	register_method("logOn", &SteamServer::logOn);
	register_method("logOnAnonymous", &SteamServer::logOnAnonymous);
	register_method("requestUserGroupStatus", &SteamServer::requestUserGroupStatus);
	register_method("secure", &SteamServer::secure);
	register_method("setAdvertiseServerActive", &SteamServer::setAdvertiseServerActive);
	register_method("setBotPlayerCount", &SteamServer::setBotPlayerCount);
	register_method("setDedicatedServer", &SteamServer::setDedicatedServer);
	register_method("setGameData", &SteamServer::setGameData);
	register_method("setGameDescription", &SteamServer::setGameDescription);
	register_method("setGameTags", &SteamServer::setGameTags);
	register_method("setKeyValue", &SteamServer::setKeyValue);
	register_method("setMapName", &SteamServer::setMapName);
	register_method("setMaxPlayerCount", &SteamServer::setMaxPlayerCount);
	register_method("setModDir", &SteamServer::setModDir);
	register_method("setPasswordProtected", &SteamServer::setPasswordProtected);
	register_method("setProduct", &SteamServer::setProduct);
	register_method("setRegion", &SteamServer::setRegion);
	register_method("setServerName", &SteamServer::setServerName);
	register_method("setSpectatorPort", &SteamServer::setSpectatorPort);
	register_method("setSpectatorServerName", &SteamServer::setSpectatorServerName);
	register_method("userHasLicenceForApp", &SteamServer::userHasLicenceForApp);
	register_method("wasRestartRequested", &SteamServer::wasRestartRequested);	
	
	// GAME SERVER STATS BIND METHODS ///////////
	register_method("clearUserAchievement", &SteamServer::clearUserAchievement);
	register_method("getUserAchievement", &SteamServer::getUserAchievement);
	register_method("getUserStatInt", &SteamServer::getUserStatInt);
	register_method("getUserStatFloat", &SteamServer::getUserStatFloat);
	register_method("requestUserStats", &SteamServer::requestUserStats);
	register_method("setUserAchievement", &SteamServer::setUserAchievement);
	register_method("setUserStatInt", &SteamServer::setUserStatInt);
	register_method("setUserStatFloat", &SteamServer::setUserStatFloat);
	register_method("storeUserStats", &SteamServer::storeUserStats);
	register_method("updateUserAvgRateStat", &SteamServer::updateUserAvgRateStat);

	// HTTP BIND METHODS ////////////////////////
	register_method("createCookieContainer", &SteamServer::createCookieContainer);
	register_method("createHTTPRequest", &SteamServer::createHTTPRequest);
	register_method("deferHTTPRequest", &SteamServer::deferHTTPRequest);
	register_method("getHTTPDownloadProgressPct", &SteamServer::getHTTPDownloadProgressPct);
	register_method("getHTTPRequestWasTimedOut", &SteamServer::getHTTPRequestWasTimedOut);
	register_method("getHTTPResponseBodyData", &SteamServer::getHTTPResponseBodyData);
	register_method("getHTTPResponseBodySize", &SteamServer::getHTTPResponseBodySize);
	register_method("getHTTPResponseHeaderSize", &SteamServer::getHTTPResponseHeaderSize);
	register_method("getHTTPResponseHeaderValue", &SteamServer::getHTTPResponseHeaderValue);
	register_method("getHTTPStreamingResponseBodyData", &SteamServer::getHTTPStreamingResponseBodyData);
	register_method("prioritizeHTTPRequest", &SteamServer::prioritizeHTTPRequest);
	register_method("releaseCookieContainer", &SteamServer::releaseCookieContainer);
	register_method("releaseHTTPRequest", &SteamServer::releaseHTTPRequest);
	register_method("sendHTTPRequest", &SteamServer::sendHTTPRequest);
	register_method("sendHTTPRequestAndStreamResponse", &SteamServer::sendHTTPRequestAndStreamResponse);
	register_method("setHTTPCookie", &SteamServer::setHTTPCookie);
	register_method("setHTTPRequestAbsoluteTimeoutMS", &SteamServer::setHTTPRequestAbsoluteTimeoutMS);
	register_method("setHTTPRequestContextValue", &SteamServer::setHTTPRequestContextValue);
	register_method("setHTTPRequestCookieContainer", &SteamServer::setHTTPRequestCookieContainer);
	register_method("setHTTPRequestGetOrPostParameter", &SteamServer::setHTTPRequestGetOrPostParameter);
	register_method("setHTTPRequestHeaderValue", &SteamServer::setHTTPRequestHeaderValue);
	register_method("setHTTPRequestNetworkActivityTimeout", &SteamServer::setHTTPRequestNetworkActivityTimeout);
	register_method("setHTTPRequestRawPostBody", &SteamServer::setHTTPRequestRawPostBody);
	register_method("setHTTPRequestRequiresVerifiedCertificate", &SteamServer::setHTTPRequestRequiresVerifiedCertificate);
	register_method("setHTTPRequestUserAgentInfo", &SteamServer::setHTTPRequestUserAgentInfo);

	// INVENTORY BIND METHODS ///////////////////
	register_method("addPromoItem", &SteamServer::addPromoItem);
	register_method("addPromoItems", &SteamServer::addPromoItems);
	register_method("checkResultSteamID", &SteamServer::checkResultSteamID);
	register_method("consumeItem", &SteamServer::consumeItem);
	register_method("deserializeResult", &SteamServer::deserializeResult);
	register_method("destroyResult", &SteamServer::destroyResult);
	register_method("exchangeItems", &SteamServer::exchangeItems);
	register_method("generateItems", &SteamServer::generateItems);
	register_method("getAllItems", &SteamServer::getAllItems);
	register_method("getItemDefinitionProperty", &SteamServer::getItemDefinitionProperty);
	register_method("getItemsByID", &SteamServer::getItemsByID);
	register_method("getItemPrice", &SteamServer::getItemPrice);
	register_method("getItemsWithPrices", &SteamServer::getItemsWithPrices);
	register_method("getResultItemProperty", &SteamServer::getResultItemProperty);
	register_method("getResultItems", &SteamServer::getResultItems);
	register_method("getResultStatus", &SteamServer::getResultStatus);
	register_method("getResultTimestamp", &SteamServer::getResultTimestamp);
	register_method("grantPromoItems", &SteamServer::grantPromoItems);
	register_method("loadItemDefinitions", &SteamServer::loadItemDefinitions);
	register_method("requestEligiblePromoItemDefinitionsIDs", &SteamServer::requestEligiblePromoItemDefinitionsIDs);
	register_method("requestPrices", &SteamServer::requestPrices);
	register_method("serializeResult", &SteamServer::serializeResult);
	register_method("startPurchase", &SteamServer::startPurchase);
	register_method("transferItemQuantity", &SteamServer::transferItemQuantity);
	register_method("triggerItemDrop", &SteamServer::triggerItemDrop);
	register_method("startUpdateProperties", &SteamServer::startUpdateProperties);
	register_method("submitUpdateProperties", &SteamServer::submitUpdateProperties);
	register_method("removeProperty", &SteamServer::removeProperty);
	register_method("setPropertyString", &SteamServer::setPropertyString);
	register_method("setPropertyBool", &SteamServer::setPropertyBool);
	register_method("setPropertyInt", &SteamServer::setPropertyInt);
	register_method("setPropertyFloat", &SteamServer::setPropertyFloat);

	// NETWORKING BIND METHODS //////////////////
	register_method("acceptP2PSessionWithUser", &SteamServer::acceptP2PSessionWithUser);
	register_method("allowP2PPacketRelay", &SteamServer::allowP2PPacketRelay);
	register_method("closeP2PChannelWithUser", &SteamServer::closeP2PChannelWithUser);
	register_method("closeP2PSessionWithUser", &SteamServer::closeP2PSessionWithUser);
	register_method("getP2PSessionState", &SteamServer::getP2PSessionState);
	register_method("getAvailableP2PPacketSize", &SteamServer::getAvailableP2PPacketSize);
	register_method("readP2PPacket", &SteamServer::readP2PPacket);
	register_method("sendP2PPacket", &SteamServer::sendP2PPacket);

	// NETWORKING MESSAGES BIND METHODS /////////
	register_method("acceptSessionWithUser", &SteamServer::acceptSessionWithUser);
	register_method("closeChannelWithUser", &SteamServer::closeChannelWithUser);
	register_method("closeSessionWithUser", &SteamServer::closeSessionWithUser);
	register_method("getSessionConnectionInfo", &SteamServer::getSessionConnectionInfo);
	register_method("receiveMessagesOnChannel", &SteamServer::receiveMessagesOnChannel);
	register_method("sendMessageToUser", &SteamServer::sendMessageToUser);
	
	// NETWORKING SOCKETS BIND METHODS //////////
	register_method("acceptConnection", &SteamServer::acceptConnection);
	register_method("beginAsyncRequestFakeIP", &SteamServer::beginAsyncRequestFakeIP);
	register_method("closeConnection", &SteamServer::closeConnection);
	register_method("closeListenSocket", &SteamServer::closeListenSocket);
	register_method("configureConnectionLanes", &SteamServer::configureConnectionLanes);
	register_method("connectP2P", &SteamServer::connectP2P);
	register_method("connectByIPAddress", &SteamServer::connectByIPAddress);
	register_method("connectToHostedDedicatedServer", &SteamServer::connectToHostedDedicatedServer);
	register_method("createFakeUDPPort", &SteamServer::createFakeUDPPort);
	register_method("createHostedDedicatedServerListenSocket", &SteamServer::createHostedDedicatedServerListenSocket);
	register_method("createListenSocketIP", &SteamServer::createListenSocketIP);
	register_method("createListenSocketP2P", &SteamServer::createListenSocketP2P);
	register_method("createListenSocketP2PFakeIP", &SteamServer::createListenSocketP2PFakeIP);
	register_method("createPollGroup", &SteamServer::createPollGroup);
	register_method("createSocketPair", &SteamServer::createSocketPair);
	register_method("destroyPollGroup", &SteamServer::destroyPollGroup);
//	register_method("findRelayAuthTicketForServer", &SteamServer::findRelayAuthTicketForServer);	<------ Uses datagram relay structs which were removed from base SDK
	register_method("flushMessagesOnConnection", &SteamServer::flushMessagesOnConnection);
	register_method("getAuthenticationStatus", &SteamServer::getAuthenticationStatus);		
	register_method("getCertificateRequest", &SteamServer::getCertificateRequest);
	register_method("getConnectionInfo", &SteamServer::getConnectionInfo);
	register_method("getConnectionName", &SteamServer::getConnectionName);
	register_method("getConnectionRealTimeStatus", &SteamServer::getConnectionRealTimeStatus);
	register_method("getConnectionUserData", &SteamServer::getConnectionUserData);
	register_method("getDetailedConnectionStatus", &SteamServer::getDetailedConnectionStatus);
	register_method("getFakeIP", &SteamServer::getFakeIP);
//	register_method("getGameCoordinatorServerLogin", &SteamServer::getGameCoordinatorServerLogin);	<------ Uses datagram relay structs which were removed from base SDK
//	register_method("getHostedDedicatedServerAddress", &SteamServer::getHostedDedicatedServerAddress);	<------ Uses datagram relay structs which were removed from base SDK
	register_method("getHostedDedicatedServerPOPId", &SteamServer::getHostedDedicatedServerPOPId);
	register_method("getHostedDedicatedServerPort", &SteamServer::getHostedDedicatedServerPort);
	register_method("getListenSocketAddress", &SteamServer::getListenSocketAddress);
	register_method("getRemoteFakeIPForConnection", &SteamServer::getRemoteFakeIPForConnection);
	register_method("initAuthentication", &SteamServer::initAuthentication);
	register_method("receiveMessagesOnConnection", &SteamServer::receiveMessagesOnConnection);	
	register_method("receiveMessagesOnPollGroup", &SteamServer::receiveMessagesOnPollGroup);
//	register_method("receivedRelayAuthTicket", &SteamServer::receivedRelayAuthTicket);	<------ Uses datagram relay structs which were removed from base SDK
	register_method("resetIdentity", &SteamServer::resetIdentity);
	register_method("runNetworkingCallbacks", &SteamServer::runNetworkingCallbacks);
	register_method("sendMessages", &SteamServer::sendMessages);
	register_method("sendMessageToConnection", &SteamServer::sendMessageToConnection);
	register_method("setCertificate", &SteamServer::setCertificate);
	register_method("setConnectionPollGroup", &SteamServer::setConnectionPollGroup);
	register_method("setConnectionName", &SteamServer::setConnectionName);

	// NETWORKING UTILS BIND METHODS ////////////
	register_method("checkPingDataUpToDate", &SteamServer::checkPingDataUpToDate);
	register_method("convertPingLocationToString", &SteamServer::convertPingLocationToString);
	register_method("estimatePingTimeBetweenTwoLocations", &SteamServer::estimatePingTimeBetweenTwoLocations);
	register_method("estimatePingTimeFromLocalHost", &SteamServer::estimatePingTimeFromLocalHost);
	register_method("getConfigValue", &SteamServer::getConfigValue);
	register_method("getConfigValueInfo", &SteamServer::getConfigValueInfo);
	register_method("getDirectPingToPOP", &SteamServer::getDirectPingToPOP);
	register_method("getLocalPingLocation", &SteamServer::getLocalPingLocation);
	register_method("getLocalTimestamp", &SteamServer::getLocalTimestamp);
	register_method("getPingToDataCenter", &SteamServer::getPingToDataCenter);
	register_method("getPOPCount", &SteamServer::getPOPCount);
	register_method("getPOPList", &SteamServer::getPOPList);
	register_method("getRelayNetworkStatus", &SteamServer::getRelayNetworkStatus);
	register_method("initRelayNetworkAccess", &SteamServer::initRelayNetworkAccess);
	register_method("parsePingLocationString", &SteamServer::parsePingLocationString);
	register_method("setConnectionConfigValueFloat", &SteamServer::setConnectionConfigValueFloat);
	register_method("setConnectionConfigValueInt32", &SteamServer::setConnectionConfigValueInt32);
	register_method("setConnectionConfigValueString", &SteamServer::setConnectionConfigValueString);
//	register_method("setConfigValue", &SteamServer::setConfigValue);
	register_method("setGlobalConfigValueFloat", &SteamServer::setGlobalConfigValueFloat);
	register_method("setGlobalConfigValueInt32", &SteamServer::setGlobalConfigValueInt32);
	register_method("setGlobalConfigValueString", &SteamServer::setGlobalConfigValueString);

	// UGC BIND METHODS ////////////////////
	register_method("addAppDependency", &SteamServer::addAppDependency);
	register_method("addContentDescriptor", &SteamServer::addContentDescriptor);
	register_method("addDependency", &SteamServer::addDependency);
	register_method("addExcludedTag", &SteamServer::addExcludedTag);
	register_method("addItemKeyValueTag", &SteamServer::addItemKeyValueTag);
	register_method("addItemPreviewFile", &SteamServer::addItemPreviewFile);
	register_method("addItemPreviewVideo", &SteamServer::addItemPreviewVideo);
	register_method("addItemToFavorites", &SteamServer::addItemToFavorites);
	register_method("addRequiredKeyValueTag", &SteamServer::addRequiredKeyValueTag);
	register_method("addRequiredTag", &SteamServer::addRequiredTag);
	register_method("addRequiredTagGroup",  &SteamServer::addRequiredTagGroup);
	register_method("initWorkshopForGameServer", &SteamServer::initWorkshopForGameServer);
	register_method("createItem", &SteamServer::createItem);
	register_method("createQueryAllUGCRequest", &SteamServer::createQueryAllUGCRequest);
	register_method("createQueryUGCDetailsRequest", &SteamServer::createQueryUGCDetailsRequest);
	register_method("createQueryUserUGCRequest", &SteamServer::createQueryUserUGCRequest);
	register_method("deleteItem", &SteamServer::deleteItem);
	register_method("downloadItem", &SteamServer::downloadItem);
	register_method("getItemDownloadInfo",  &SteamServer::getItemDownloadInfo);
	register_method("getItemInstallInfo", &SteamServer::getItemInstallInfo);
	register_method("getItemState", &SteamServer::getItemState);
	register_method("getItemUpdateProgress", &SteamServer::getItemUpdateProgress);
	register_method("getNumSubscribedItems", &SteamServer::getNumSubscribedItems);
	register_method("getQueryUGCAdditionalPreview", &SteamServer::getQueryUGCAdditionalPreview);
	register_method("getQueryUGCChildren", &SteamServer::getQueryUGCChildren);
	register_method("getQueryUGCContentDescriptors", &SteamServer::getQueryUGCContentDescriptors);
	register_method("getQueryUGCKeyValueTag", &SteamServer::getQueryUGCKeyValueTag);
	register_method("getQueryUGCMetadata", &SteamServer::getQueryUGCMetadata);
	register_method("getQueryUGCNumAdditionalPreviews",  &SteamServer::getQueryUGCNumAdditionalPreviews);
	register_method("getQueryUGCNumKeyValueTags",  &SteamServer::getQueryUGCNumKeyValueTags);
	register_method("getQueryUGCNumTags",  &SteamServer::getQueryUGCNumTags);
	register_method("getQueryUGCPreviewURL",  &SteamServer::getQueryUGCPreviewURL);
	register_method("getQueryUGCResult",  &SteamServer::getQueryUGCResult);
	register_method("getQueryUGCStatistic", &SteamServer::getQueryUGCStatistic);
	register_method("getQueryUGCTag", &SteamServer::getQueryUGCTag);
	register_method("getQueryUGCTagDisplayName", &SteamServer::getQueryUGCTagDisplayName);
	register_method("getSubscribedItems", &SteamServer::getSubscribedItems);
	register_method("getUserContentDescriptorPreferences", &SteamServer::getUserContentDescriptorPreferences);
	register_method("getUserItemVote", &SteamServer::getUserItemVote);
	register_method("releaseQueryUGCRequest", &SteamServer::releaseQueryUGCRequest);
	register_method("removeAppDependency", &SteamServer::removeAppDependency);
	register_method("removeContentDescriptor", &SteamServer::removeContentDescriptor);
	register_method("removeDependency", &SteamServer::removeDependency);
	register_method("removeItemFromFavorites", &SteamServer::removeItemFromFavorites);
	register_method("removeItemKeyValueTags", &SteamServer::removeItemKeyValueTags);
	register_method("removeItemPreview", &SteamServer::removeItemPreview);
	register_method("sendQueryUGCRequest", &SteamServer::sendQueryUGCRequest);
	register_method("setAllowCachedResponse", &SteamServer::setAllowCachedResponse);
	register_method("setCloudFileNameFilter", &SteamServer::setCloudFileNameFilter);
	register_method("setItemContent", &SteamServer::setItemContent);
	register_method("setItemDescription", &SteamServer::setItemDescription);
	register_method("setItemMetadata", &SteamServer::setItemMetadata);
	register_method("setItemPreview", &SteamServer::setItemPreview);
	register_method("setItemTags", &SteamServer::setItemTags);
	register_method("setItemTitle", &SteamServer::setItemTitle);
	register_method("setItemUpdateLanguage", &SteamServer::setItemUpdateLanguage);
	register_method("setItemVisibility", &SteamServer::setItemVisibility);
	register_method("setLanguage", &SteamServer::setLanguage);
	register_method("setMatchAnyTag", &SteamServer::setMatchAnyTag);
	register_method("setRankedByTrendDays", &SteamServer::setRankedByTrendDays);
	register_method("setReturnAdditionalPreviews", &SteamServer::setReturnAdditionalPreviews);
	register_method("setReturnChildren", &SteamServer::setReturnChildren);
	register_method("setReturnKeyValueTags", &SteamServer::setReturnKeyValueTags);
	register_method("setReturnLongDescription", &SteamServer::setReturnLongDescription);
	register_method("setReturnMetadata", &SteamServer::setReturnMetadata);
	register_method("setReturnOnlyIDs", &SteamServer::setReturnOnlyIDs);
	register_method("setReturnPlaytimeStats", &SteamServer::setReturnPlaytimeStats);
	register_method("setReturnTotalOnly", &SteamServer::setReturnTotalOnly);
	register_method("setSearchText", &SteamServer::setSearchText);
	register_method("setUserItemVote", &SteamServer::setUserItemVote);
	register_method("startItemUpdate", &SteamServer::startItemUpdate);
	register_method("startPlaytimeTracking", &SteamServer::startPlaytimeTracking);
	register_method("stopPlaytimeTracking", &SteamServer::stopPlaytimeTracking);
	register_method("stopPlaytimeTrackingForAllItems", &SteamServer::stopPlaytimeTrackingForAllItems);
	register_method("getAppDependencies", &SteamServer::getAppDependencies);
	register_method("submitItemUpdate", &SteamServer::submitItemUpdate);
	register_method("subscribeItem", &SteamServer::subscribeItem);
	register_method("suspendDownloads", &SteamServer::suspendDownloads);
	register_method("unsubscribeItem", &SteamServer::unsubscribeItem);
	register_method("updateItemPreviewFile", &SteamServer::updateItemPreviewFile);
	register_method("updateItemPreviewVideo", &SteamServer::updateItemPreviewVideo);
	register_method("showWorkshopEULA", &SteamServer::showWorkshopEULA);	
	register_method("getWorkshopEULAStatus", &SteamServer::getWorkshopEULAStatus);
	register_method("setTimeCreatedDateRange", &SteamServer::setTimeCreatedDateRange);
	register_method("setTimeUpdatedDateRange", &SteamServer::setTimeUpdatedDateRange);


	// CALLBACK SIGNAL BINDS
	/////////////////////////////////////////////
	//
	// STEAMWORKS SIGNALS ///////////////////////
	register_signal<SteamServer>("steamworks_error", "failed_signal", GODOT_VARIANT_TYPE_STRING, "io failure", GODOT_VARIANT_TYPE_STRING);

	// GAME SERVER SIGNALS //////////////////////
	register_signal<SteamServer>("associate_clan", "result", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("client_approved", "steam_id", GODOT_VARIANT_TYPE_INT, "owner_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("client_denied", "steam_id", GODOT_VARIANT_TYPE_INT, "reason", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("client_group_status", "steam_id", GODOT_VARIANT_TYPE_INT, "group_id", GODOT_VARIANT_TYPE_INT, "member", GODOT_VARIANT_TYPE_BOOL, "officer", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("client_kick", "steam_id", GODOT_VARIANT_TYPE_INT, "reason", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("policy_response", "secure", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("player_compat", "result", GODOT_VARIANT_TYPE_INT, "players_dont_like_candidate", GODOT_VARIANT_TYPE_INT, "players_candidate_doesnt_like", GODOT_VARIANT_TYPE_INT, "clan_players_dont_like_candidate", GODOT_VARIANT_TYPE_INT, "steam_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("server_connect_failure", "result", GODOT_VARIANT_TYPE_INT, "retrying", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("server_connected");
	register_signal<SteamServer>("server_disconnected", "result", GODOT_VARIANT_TYPE_INT);
	
	// GAME SERVER STATS SIGNALS ////////////////
	register_signal<SteamServer>("stats_received", "result", GODOT_VARIANT_TYPE_INT, "steam_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("stats_stored", "result", GODOT_VARIANT_TYPE_INT, "steam_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("stats_unloaded", "steam_id", GODOT_VARIANT_TYPE_INT);

	// HTTP SIGNALS /////////////////////////////
	register_signal<SteamServer>("http_request_completed", "cookie_handle", GODOT_VARIANT_TYPE_INT, "context_value", GODOT_VARIANT_TYPE_INT, "request_success", GODOT_VARIANT_TYPE_BOOL, "status_code", GODOT_VARIANT_TYPE_INT, "body_size", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("http_request_data_received", "cookie_handle", GODOT_VARIANT_TYPE_INT, "context_value", GODOT_VARIANT_TYPE_INT, "offset", GODOT_VARIANT_TYPE_INT, "bytes_received", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("http_request_headers_received", "cookie_handle", GODOT_VARIANT_TYPE_INT, "context_value", GODOT_VARIANT_TYPE_INT);

	// INVENTORY SIGNALS ////////////////////////
	register_signal<SteamServer>("inventory_definition_update", "definitions", GODOT_VARIANT_TYPE_ARRAY);
	register_signal<SteamServer>("inventory_eligible_promo_item", "result", GODOT_VARIANT_TYPE_INT, "cached", GODOT_VARIANT_TYPE_BOOL, "definitions", GODOT_VARIANT_TYPE_ARRAY);
	register_signal<SteamServer>("inventory_full_update", "inventory_handle", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("inventory_result_ready", "result", GODOT_VARIANT_TYPE_INT, "inventory_handle", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("inventory_start_purchase_result", "result", GODOT_VARIANT_TYPE_STRING, "order_id", GODOT_VARIANT_TYPE_INT, "transaction_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("inventory_request_prices_result", "result", GODOT_VARIANT_TYPE_INT, "currency", GODOT_VARIANT_TYPE_STRING);

	// NETWORKING SIGNALS ///////////////////////
	register_signal<SteamServer>("p2p_session_request", "remote_steam_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("p2p_session_connect_fail", "remote_steam_id", GODOT_VARIANT_TYPE_INT, "session_error", GODOT_VARIANT_TYPE_INT);

	// NETWORKING MESSAGES //////////////////////
	register_signal<SteamServer>("network_messages_session_request", "identity", GODOT_VARIANT_TYPE_STRING);
	register_signal<SteamServer>("network_messages_session_failed", "reason", GODOT_VARIANT_TYPE_INT);

	// NETWORKING SOCKETS SIGNALS ///////////////
	register_signal<SteamServer>("network_connection_status_changed", "connect_handle", GODOT_VARIANT_TYPE_INT, "connection", GODOT_VARIANT_TYPE_DICTIONARY, "old_state", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("network_authentication_status", "available", GODOT_VARIANT_TYPE_INT, "debug_message", GODOT_VARIANT_TYPE_STRING);
	register_signal<SteamServer>("fake_ip_result", "result", GODOT_VARIANT_TYPE_INT, "identity", GODOT_VARIANT_TYPE_STRING, "fake_ip", GODOT_VARIANT_TYPE_STRING, "port_list", GODOT_VARIANT_TYPE_ARRAY);

	// NETWORKING UTILS SIGNALS /////////////////
	register_signal<SteamServer>("relay_network_status", "available", GODOT_VARIANT_TYPE_INT, "ping_measurement", GODOT_VARIANT_TYPE_INT, "available_config", GODOT_VARIANT_TYPE_INT, "available_relay", GODOT_VARIANT_TYPE_INT, "debug_message", GODOT_VARIANT_TYPE_STRING);

	// REMOTE STORAGE SIGNALS ///////////////////
	register_signal<SteamServer>("file_read_async_complete", "file_read", GODOT_VARIANT_TYPE_DICTIONARY);
	register_signal<SteamServer>("file_share_result", "result", GODOT_VARIANT_TYPE_INT, "handle", GODOT_VARIANT_TYPE_INT, "name", GODOT_VARIANT_TYPE_STRING);
	register_signal<SteamServer>("file_write_async_complete", "result", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("download_ugc_result", "result", GODOT_VARIANT_TYPE_INT, "download_data", GODOT_VARIANT_TYPE_DICTIONARY);
	register_signal<SteamServer>("unsubscribe_item", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("subscribe_item", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("local_file_changed");

	// UGC SIGNALS //////////////////////////////
	register_signal<SteamServer>("add_app_dependency_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "app_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("add_ugc_dependency_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "child_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("item_created", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "accept_tos", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("item_downloaded", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "app_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("get_app_dependencies_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "app_dependencies", GODOT_VARIANT_TYPE_INT, "total_app_dependencies", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("item_deleted", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("get_item_vote_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "vote_up", GODOT_VARIANT_TYPE_BOOL, "vote_down", GODOT_VARIANT_TYPE_BOOL, "vote_skipped", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("item_installed", "app_id", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("remove_app_dependency_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "app_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("remove_ugc_dependency_result", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "child_id", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("set_user_item_vote", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "vote_up", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("start_playtime_traciking", "result", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("ugc_query_completed", "handle", GODOT_VARIANT_TYPE_INT, "result", GODOT_VARIANT_TYPE_INT, "results_returned", GODOT_VARIANT_TYPE_INT, "total_matching", GODOT_VARIANT_TYPE_INT, "cached", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("stop_playtime_tracking", "result", GODOT_VARIANT_TYPE_INT);
	register_signal<SteamServer>("item_updated", "result", GODOT_VARIANT_TYPE_INT, "accept_tos", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("user_favorite_items_list_changed", "result", GODOT_VARIANT_TYPE_INT, "file_id", GODOT_VARIANT_TYPE_INT, "was_add_request", GODOT_VARIANT_TYPE_BOOL);
	register_signal<SteamServer>("workshop_eula_status", "result", GODOT_VARIANT_TYPE_INT, "app_id", GODOT_VARIANT_TYPE_INT, "eula_data", GODOT_VARIANT_TYPE_DICTIONARY);
	register_signal<SteamServer>("user_subscribed_items_list_changed", "app_id", GODOT_VARIANT_TYPE_INT);
}

SteamServer::~SteamServer(){
	if(is_init_success){
		SteamGameServer_Shutdown();
	}
}
