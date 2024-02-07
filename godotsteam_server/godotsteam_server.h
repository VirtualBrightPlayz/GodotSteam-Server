#ifndef GODOTSTEAM_SERVER_H
#define GODOTSTEAM_SERVER_H

/////////////////////////////////////////////////
// SILENCE STEAMWORKS WARNINGS
/////////////////////////////////////////////////
//
// Turn off MSVC-only warning about strcpy
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable:4996)
#pragma warning(disable:4828)
#endif

/////////////////////////////////////////////////
// INCLUDE HEADERS
/////////////////////////////////////////////////
//
// Include INT types header
#include <inttypes.h>

// Include Steamworks Server API header
#include "steam/steam_gameserver.h"
#include "steam/steamnetworkingfakeip.h"

// Include Godot headers
#include "Godot.hpp"
#include "Object.hpp"
#include "Texture.hpp"
#include "String.hpp"
#include "Reference.hpp"
#include "Dictionary.hpp"
#include "IP.hpp"
//#include "core/method_bind_ext.gen.inc"

// Include some system headers
#include "map"

namespace godot {

	class SteamServer: public Reference {
		GODOT_CLASS(SteamServer, Reference);
	
	public:

		static void _register_methods();
		void _init(){ }

		SteamServer();
		~SteamServer();

		/////////////////////////////////////////
		// STEAMWORKS FUNCTIONS /////////////////
		/////////////////////////////////////////
		//
		CSteamID createSteamID(uint64_t steam_id);

		// Main /////////////////////////////////
		bool isServerSecure();
		uint64_t getServerSteamID();
		bool serverInit(String ip, uint16 game_port, uint16 query_port, int server_mode, String version_number);
		Dictionary serverInitEx(String ip, uint16 game_port, uint16 query_port, int server_mode, String version_number);
		void serverReleaseCurrentThreadMemory();
		void serverShutdown();
		void steamworksError(String failed_signal);

		// Game Server //////////////////////////
		void associateWithClan(uint64_t clan_id);
		uint32 beginAuthSession(PoolByteArray ticket, int ticket_size, uint64_t steam_id);
		void cancelAuthTicket(uint32_t auth_ticket);
		void clearAllKeyValues();
		void computeNewPlayerCompatibility(uint64_t steam_id);
		void endAuthSession(uint64_t steam_id);
		Dictionary getAuthSessionTicket(String identity_reference);
		Dictionary getNextOutgoingPacket();
		Dictionary getPublicIP();
		uint64_t getSteamID();
		Dictionary handleIncomingPacket(int packet, String ip, uint16 port);
		bool loggedOn();
		void logOff();
		void logOn(String token);
		void logOnAnonymous();
		bool requestUserGroupStatus(uint64_t steam_id, int group_id);
		bool secure();
		void setAdvertiseServerActive(bool active);
		void setBotPlayerCount(int bots);
		void setDedicatedServer(bool dedicated);
		void setGameData(String data);
		void setGameDescription(String description);
		void setGameTags(String tags);
		void setKeyValue(String key, String value);
		void setMapName(String map);
		void setMaxPlayerCount(int players_max);
		void setModDir(String mod_directory);
		void setPasswordProtected(bool password_protected);
		void setProduct(String product);
		void setRegion(String region);
		void setServerName(String name);
		void setSpectatorPort(uint16 port);
		void setSpectatorServerName(String name);
		int userHasLicenceForApp(uint64_t steam_id, uint32 app_id);
		bool wasRestartRequested();

		// Game Server Stats ////////////////////
		bool clearUserAchievement(uint64_t steam_id, String name);
		Dictionary getUserAchievement(uint64_t steam_id, String name);
		uint32_t getUserStatInt(uint64_t steam_id, String name);
		float getUserStatFloat(uint64_t steam_id, String name);
		void requestUserStats(uint64_t steam_id);
		bool setUserAchievement(uint64_t steam_id, String name);
		bool setUserStatInt(uint64_t steam_id, String name, int32 stat);
		bool setUserStatFloat(uint64_t steam_id, String name, float stat);
		void storeUserStats(uint64_t steam_id);
		bool updateUserAvgRateStat(uint64_t steam_id, String name, float this_session, double session_length);

		// HTTP /////////////////////////////////
		uint32_t createCookieContainer( bool allow_responses_to_modify);
		uint32_t createHTTPRequest(int request_method, String absolute_url);
		bool deferHTTPRequest(uint32 request_handle);
		float getHTTPDownloadProgressPct(uint32 request_handle);
		bool getHTTPRequestWasTimedOut(uint32 request_handle);
		PoolByteArray getHTTPResponseBodyData(uint32 request_handle, uint32 buffer_size);
		uint32 getHTTPResponseBodySize(uint32 request_handle);
		uint32 getHTTPResponseHeaderSize(uint32 request_handle, String header_name);
		uint8 getHTTPResponseHeaderValue(uint32 request_handle, String header_name, uint32 buffer_size);
		uint8 getHTTPStreamingResponseBodyData(uint32 request_handle, uint32 offset, uint32 buffer_size);
		bool prioritizeHTTPRequest(uint32 request_handle);
		bool releaseCookieContainer(uint32 cookie_handle);
		bool releaseHTTPRequest(uint32 request_handle);
		bool sendHTTPRequest(uint32 request_handle);
		bool sendHTTPRequestAndStreamResponse(uint32 request_handle);
		bool setHTTPCookie(uint32 cookie_handle, String host, String url, String cookie);
		bool setHTTPRequestAbsoluteTimeoutMS(uint32 request_handle, uint32 milliseconds);
		bool setHTTPRequestContextValue(uint32 request_handle, uint64_t context_value);
		bool setHTTPRequestCookieContainer(uint32 request_handle, uint32 cookie_handle);
		bool setHTTPRequestGetOrPostParameter(uint32 request_handle, String name, String value);
		bool setHTTPRequestHeaderValue(uint32 request_handle, String header_name, String header_value);
		bool setHTTPRequestNetworkActivityTimeout(uint32 request_handle, uint32 timeout_seconds);
		uint8 setHTTPRequestRawPostBody(uint32 request_handle, String content_type, uint32 body_length);
		bool setHTTPRequestRequiresVerifiedCertificate(uint32 request_handle, bool require_verified_certificate);
		bool setHTTPRequestUserAgentInfo(uint32 request_handle, String user_agent_info);

		// Inventory ////////////////////////////
		int32 addPromoItem(uint32 item);
		int32 addPromoItems(PoolIntArray items);
		bool checkResultSteamID(uint64_t steam_id_expected, int32 this_inventory_handle = 0);
		int32 consumeItem(uint64_t item_consume, uint32 quantity);
		int32 deserializeResult(PoolByteArray buffer);
		void destroyResult(int32 this_inventory_handle = 0);
		int32 exchangeItems(PoolIntArray output_items, PoolIntArray output_quantity, PoolIntArray input_items, PoolIntArray input_quantity);
		int32 generateItems(PoolIntArray items, PoolIntArray quantity);
		int32 getAllItems();
		String getItemDefinitionProperty(uint32 definition, String name);
		int32 getItemsByID(PoolIntArray id_array);
		uint64_t getItemPrice(uint32 definition);
		Array getItemsWithPrices();
		String getResultItemProperty(uint32 index, String name, int32 this_inventory_handle = 0);
		Array getResultItems(int32 this_inventory_handle = 0);
		String getResultStatus(int32 this_inventory_handle = 0);
		uint32 getResultTimestamp(int32 this_inventory_handle = 0);
		int32 grantPromoItems();
		bool loadItemDefinitions();
		void requestEligiblePromoItemDefinitionsIDs(uint64_t steam_id);
		void requestPrices();
		String serializeResult(int32 this_inventory_handle = 0);
		void startPurchase(PoolIntArray items, PoolIntArray quantity);
		int32 transferItemQuantity(uint64_t item_id, uint32 quantity, uint64_t item_destination, bool split);
		int32 triggerItemDrop(uint32 definition);
		void startUpdateProperties();
		int32 submitUpdateProperties(uint64_t this_inventory_update_handle = 0);
		bool removeProperty(uint64_t item_id, String name, uint64_t this_inventory_update_handle = 0);
		bool setPropertyString(uint64_t item_id, String name, String value, uint64_t this_inventory_update_handle = 0);
		bool setPropertyBool(uint64_t item_id, String name, bool value, uint64_t this_inventory_update_handle = 0);
		bool setPropertyInt(uint64_t item_id, String name, uint64_t value, uint64_t this_inventory_update_handle = 0);
		bool setPropertyFloat(uint64_t item_id, String name, float value, uint64_t this_inventory_update_handle = 0);

		// Networking ///////////////////////////
		bool acceptP2PSessionWithUser(uint64_t steam_id_remote);
		bool allowP2PPacketRelay(bool allow);
		bool closeP2PChannelWithUser(uint64_t steam_id_remote, int channel);
		bool closeP2PSessionWithUser(uint64_t steam_id_remote);
		Dictionary getP2PSessionState(uint64_t steam_id_remote);
		uint32_t getAvailableP2PPacketSize(int channel = 0);
		Dictionary readP2PPacket(uint32_t packet, int channel = 0);
		bool sendP2PPacket(uint64_t steam_id_remote, const PoolByteArray data, int send_type, int channel = 0);

		// Networking Messages //////////////////
		bool acceptSessionWithUser(String identity_reference);
		bool closeChannelWithUser(String identity_reference, int channel);
		bool closeSessionWithUser(String identity_reference);
		Dictionary getSessionConnectionInfo(String identity_reference, bool get_connection, bool get_status);
		Array receiveMessagesOnChannel(int channel, int max_messages);
		int sendMessageToUser(String identity_reference, const PoolByteArray data, int flags, int channel);
		
		// Networking Sockets ///////////////////
		int acceptConnection(uint32 connection_handle);
		bool beginAsyncRequestFakeIP(int num_ports);
		bool closeConnection(uint32 peer, int reason, String debug_message, bool linger);
		bool closeListenSocket(uint32 socket);
		int configureConnectionLanes(uint32 connection, int lanes, Array priorities, Array weights);
		uint32 connectP2P(String identity_reference, int virtual_port, Array options);
		uint32 connectByIPAddress(String ip_address_with_port, Array options);
		uint32 connectToHostedDedicatedServer(String identity_reference, int virtual_port, Array options);
		void createFakeUDPPort(int fake_server_port);
		uint32 createHostedDedicatedServerListenSocket(int virtual_port, Array options);
		uint32 createListenSocketIP(String ip_reference, Array options);
		uint32 createListenSocketP2P(int virtual_port, Array options);
		uint32 createListenSocketP2PFakeIP(int fake_port, Array options);
		uint32 createPollGroup();
		Dictionary createSocketPair(bool loopback, String identity_reference1, String identity_reference2);
		bool destroyPollGroup(uint32 poll_group);
//		int findRelayAuthTicketForServer(int port);	<------ Uses datagram relay structs which were removed from base SDK
		int flushMessagesOnConnection(uint32 connection_handle);
		int getAuthenticationStatus();
		Dictionary getCertificateRequest();
		Dictionary getConnectionInfo(uint32 connection_handle);
		String getConnectionName(uint32 peer);
		Dictionary getConnectionRealTimeStatus(uint32 connection_handle, int lanes, bool get_status = true);
		uint64_t getConnectionUserData(uint32 peer);
		Dictionary getDetailedConnectionStatus(uint32 connection_handle);
		Dictionary getFakeIP(int first_port = 0);
//		int getGameCoordinatorServerLogin(String app_data);	<------ Uses datagram relay structs which were removed from base SDK
//		int getHostedDedicatedServerAddress();	<------ Uses datagram relay structs which were removed from base SDK
		uint32 getHostedDedicatedServerPOPId();
		uint16 getHostedDedicatedServerPort();
		String getListenSocketAddress(uint32 socket, bool with_port = true);
		String getIdentity();
		Dictionary getRemoteFakeIPForConnection(uint32 connection);
		int initAuthentication();
		Array receiveMessagesOnConnection(uint32 connection, int max_messages);
		Array receiveMessagesOnPollGroup(uint32 poll_group, int max_messages);
//		Dictionary receivedRelayAuthTicket();	<------ Uses datagram relay structs which were removed from base SDK
		void resetIdentity(String this_identity);
		void runNetworkingCallbacks();
		void sendMessages(int messages, const PoolByteArray data, uint32 connection_handle, int flags);
		Dictionary sendMessageToConnection(uint32 connection_handle, const PoolByteArray data, int flags);
		Dictionary setCertificate(const PoolByteArray& certificate);		
		bool setConnectionPollGroup(uint32 connection_handle, uint32 poll_group);
		void setConnectionName(uint32 peer, String name);

		// Networking Types /////////////////////
		bool addIdentity(String reference_name);
		bool addIPAddress(String reference_name);
		void clearIdentity(String reference_name);
		void clearIPAddress(String reference_name);
		uint8 getGenericBytes(String reference_name);
		String getGenericString(String reference_name);
		Array getIdentities();
		uint32 getIdentityIPAddr(String reference_name);
		uint32 getIdentitySteamID(String reference_name);
		uint64_t getIdentitySteamID64(String reference_name);
		Array getIPAddresses();
		uint32 getIPv4(String reference_name);
		uint64_t getPSNID(String reference_name);
		uint64_t getStadiaID(String reference_name);
		String getXboxPairwiseID(String reference_name);
		bool isAddressLocalHost(String reference_name);
		bool isIdentityInvalid(String reference_name);
		bool isIdentityLocalHost(String reference_name);
		bool isIPv4(String reference_name);
		bool isIPv6AllZeros(String reference_name);
		bool parseIdentityString(String reference_name, String string_to_parse);
		bool parseIPAddressString(String reference_name, String string_to_parse);
		bool setGenericBytes(String reference_name, uint8 data);
		bool setGenericString(String reference_name, String this_string);
		bool setIdentityIPAddr(String reference_name, String ip_address_name);
		void setIdentityLocalHost(String reference_name);
		void setIdentitySteamID(String reference_name, uint32 steam_id);
		void setIdentitySteamID64(String reference_name, uint64_t steam_id);
		void setIPv4(String reference_name, uint32 ip, uint16 port);
		void setIPv6(String reference_name, uint8 ipv6, uint16 port);
		void setIPv6LocalHost(String reference_name, uint16 port = 0);
		void setPSNID(String reference_name, uint64_t psn_id);
		void setStadiaID(String reference_name, uint64_t stadia_id);
		bool setXboxPairwiseID(String reference_name, String xbox_id);
		String toIdentityString(String reference_name);
		String toIPAddressString(String reference_name, bool with_port);
		const SteamNetworkingConfigValue_t* convertOptionsArray(Array options);
		
		// Networking Utils /////////////////////
		bool checkPingDataUpToDate(float max_age_in_seconds);
		String convertPingLocationToString(PoolByteArray location);
		int estimatePingTimeBetweenTwoLocations(PoolByteArray location1, PoolByteArray location2);
		int estimatePingTimeFromLocalHost(PoolByteArray location);
		Dictionary getConfigValue(int config_value, int scope_type, uint32_t connection_handle);
		Dictionary getConfigValueInfo(int config_value);
		int getDirectPingToPOP(uint32 pop_id);
		Dictionary getLocalPingLocation();
		uint64_t getLocalTimestamp();
		Dictionary getPingToDataCenter(uint32 pop_id);
		int getPOPCount();
		Array getPOPList();
		int getRelayNetworkStatus();
		void initRelayNetworkAccess();
		Dictionary parsePingLocationString(String location_string);
		bool setConnectionConfigValueFloat(uint32 connection, int config, float value);
		bool setConnectionConfigValueInt32(uint32 connection, int config, int32 value);
		bool setConnectionConfigValueString(uint32 connection, int config, String value);
//		bool setConfigValue(int setting, int scope_type, uint32_t connection_handle, int data_type, auto value);
		bool setGlobalConfigValueFloat(int config, float value);		
		bool setGlobalConfigValueInt32(int config, int32 value);
		bool setGlobalConfigValueString(int config, String value);

		// UGC //////////////////////////////////
		void addAppDependency(uint64_t published_file_id, uint32_t app_id);
		bool addContentDescriptor(uint64_t update_handle, int descriptor_id);
		void addDependency(uint64_t published_file_id, uint64_t child_published_file_id);
		bool addExcludedTag(uint64_t query_handle, String tag_name);
		bool addItemKeyValueTag(uint64_t query_handle, String key, String value);
		bool addItemPreviewFile(uint64_t query_handle, String preview_file, int type);
		bool addItemPreviewVideo(uint64_t query_handle, String video_id);
		void addItemToFavorites(uint32_t app_id, uint64_t published_file_id);
		bool addRequiredKeyValueTag(uint64_t query_handle, String key, String value);
		bool addRequiredTag(uint64_t query_handle, String tag_name);
		bool addRequiredTagGroup(uint64_t query_handle, Array tag_array);
		bool initWorkshopForGameServer(uint32_t workshop_depot_id);
		void createItem(uint32 app_id, int file_type);
		uint64_t createQueryAllUGCRequest(int query_type, int matching_type, uint32_t creator_id, uint32_t consumer_id, uint32 page);
		uint64_t createQueryUGCDetailsRequest(Array published_file_id);
		uint64_t createQueryUserUGCRequest(uint64_t steam_id, int list_type, int matching_ugc_type, int sort_order, uint32_t creator_id, uint32_t consumer_id, uint32 page);
		void deleteItem(uint64_t published_file_id);
		bool downloadItem(uint64_t published_file_id, bool high_priority);
		Dictionary getItemDownloadInfo(uint64_t published_file_id);
		Dictionary getItemInstallInfo(uint64_t published_file_id);
		uint32 getItemState(uint64_t published_file_id);
		Dictionary getItemUpdateProgress(uint64_t update_handle);
		uint32 getNumSubscribedItems();
		Dictionary getQueryUGCAdditionalPreview(uint64_t query_handle, uint32 index, uint32 preview_index);
		Dictionary getQueryUGCChildren(uint64_t query_handle, uint32 index, uint32_t child_count);
		Dictionary getQueryUGCContentDescriptors(uint64_t query_handle, uint32 index, uint32_t max_entries);
		Dictionary getQueryUGCKeyValueTag(uint64_t query_handle, uint32 index, uint32 key_value_tag_index);
		String getQueryUGCMetadata(uint64_t query_handle, uint32 index);
		uint32 getQueryUGCNumAdditionalPreviews(uint64_t query_handle, uint32 index);
		uint32 getQueryUGCNumKeyValueTags(uint64_t query_handle, uint32 index);
		uint32 getQueryUGCNumTags(uint64_t query_handle, uint32 index);
		String getQueryUGCPreviewURL(uint64_t query_handle, uint32 index);
		Dictionary getQueryUGCResult(uint64_t query_handle, uint32 index);
		Dictionary getQueryUGCStatistic(uint64_t query_handle, uint32 index, int stat_type);
		String getQueryUGCTag(uint64_t query_handle, uint32 index, uint32 tag_index);
		String getQueryUGCTagDisplayName(uint64_t query_handle, uint32 index, uint32 tag_index);
		Array getSubscribedItems();
		Array getUserContentDescriptorPreferences(uint32 max_entries);
		void getUserItemVote(uint64_t published_file_id);
		bool releaseQueryUGCRequest(uint64_t query_handle);
		void removeAppDependency(uint64_t published_file_id, uint32_t app_id);
		bool removeContentDescriptor(uint64_t update_handle, int descriptor_id);
		void removeDependency(uint64_t published_file_id, uint64_t child_published_file_id);
		void removeItemFromFavorites(uint32_t app_id, uint64_t published_file_id);
		bool removeItemKeyValueTags(uint64_t update_handle, String key);
		bool removeItemPreview(uint64_t update_handle, uint32 index);
		void sendQueryUGCRequest(uint64_t update_handle);
		bool setAllowCachedResponse(uint64_t update_handle, uint32 max_age_seconds);
		bool setCloudFileNameFilter(uint64_t update_handle, String match_cloud_filename);
		bool setItemContent(uint64_t update_handle, String content_folder);
		bool setItemDescription(uint64_t update_handle, String description);
		bool setItemMetadata(uint64_t update_handle, String metadata);
		bool setItemPreview(uint64_t update_handle, String preview_file);
		bool setItemTags(uint64_t update_handle, Array tag_array, bool allow_admin_tags = false);
		bool setItemTitle(uint64_t update_handle, String title);
		bool setItemUpdateLanguage(uint64_t update_handle, String language);
		bool setItemVisibility(uint64_t update_handle, int visibility);
		bool setLanguage(uint64_t query_handle, String language);
		bool setMatchAnyTag(uint64_t query_handle, bool match_any_tag);
		bool setRankedByTrendDays(uint64_t query_handle, uint32 days);
		bool setReturnAdditionalPreviews(uint64_t query_handle, bool return_additional_previews);
		bool setReturnChildren(uint64_t query_handle, bool return_children);
		bool setReturnKeyValueTags(uint64_t query_handle, bool return_key_value_tags);
		bool setReturnLongDescription(uint64_t query_handle, bool return_long_description);
		bool setReturnMetadata(uint64_t query_handle, bool return_metadata);
		bool setReturnOnlyIDs(uint64_t query_handle, bool return_only_ids);
		bool setReturnPlaytimeStats(uint64_t query_handle, uint32 days);
		bool setReturnTotalOnly(uint64_t query_handle, bool return_total_only);
		bool setSearchText(uint64_t query_handle, String search_text);
		void setUserItemVote(uint64_t published_file_id, bool vote_up);
		uint64_t startItemUpdate(uint32_t app_id, uint64_t file_id);
		void startPlaytimeTracking(Array published_file_ids);
		void stopPlaytimeTracking(Array published_file_ids);
		void stopPlaytimeTrackingForAllItems();
		void getAppDependencies(uint64_t published_file_id);
		void submitItemUpdate(uint64_t update_handle, String change_note);
		void subscribeItem(uint64_t published_file_id);
		void suspendDownloads(bool suspend);
		void unsubscribeItem(uint64_t published_file_id);
		bool updateItemPreviewFile(uint64_t update_handle, uint32 index, String preview_file);
		bool updateItemPreviewVideo(uint64_t update_handle, uint32 index, String video_id);
		bool showWorkshopEULA();
		void getWorkshopEULAStatus();
		bool setTimeCreatedDateRange(uint64_t update_handle, uint32 start, uint32 end);
		bool setTimeUpdatedDateRange(uint64_t update_handle, uint32 start, uint32 end);

	protected:
		static void _bind_methods();
		static SteamServer* singleton;

	private:
		// Main
		bool is_init_success;

		// Inventory
		SteamInventoryUpdateHandle_t inventory_update_handle;
		SteamInventoryResult_t inventory_handle;
		SteamItemDetails_t inventory_details;

		// Networking Messages
//		std::map<int, SteamNetworkingMessage_t> network_messages;

		// Networking Sockets
		uint32 network_connection;
		uint32 listen_socket;
		uint32 network_poll_group;
		uint64_t networking_microseconds = 0;
		SteamNetworkingIdentity networking_identity;
		SteamNetworkingIdentity game_server;
//		SteamDatagramHostedAddress hosted_address;
		PoolByteArray routing_blob;
//		SteamDatagramRelayAuthTicket relay_auth_ticket;
		std::map<String, SteamNetworkingIdentity> networking_identities;
		std::map<String, SteamNetworkingIPAddr> ip_addresses;

		// Run the Steamworks server API callbacks
		void run_callbacks(){
			SteamGameServer_RunCallbacks();
		}

		/////////////////////////////////////////
		// STEAM SERVER CALLBACKS ///////////////
		/////////////////////////////////////////
		//
		// Game Server callbacks ////////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, server_connect_failure, SteamServerConnectFailure_t, callbackServerConnectFailure);
		STEAM_GAMESERVER_CALLBACK(SteamServer, server_connected, SteamServersConnected_t, callbackServerConnected);
		STEAM_GAMESERVER_CALLBACK(SteamServer, server_disconnected, SteamServersDisconnected_t, callbackServerDisconnected);
		STEAM_GAMESERVER_CALLBACK(SteamServer, client_approved, GSClientApprove_t, callbackClientApproved);
		STEAM_GAMESERVER_CALLBACK(SteamServer, client_denied, GSClientDeny_t, callbackClientDenied);
		STEAM_GAMESERVER_CALLBACK(SteamServer, client_kick, GSClientKick_t, callbackClientKicked);
		STEAM_GAMESERVER_CALLBACK(SteamServer, policy_response, GSPolicyResponse_t, callbackPolicyResponse);
		STEAM_GAMESERVER_CALLBACK(SteamServer, client_group_status, GSClientGroupStatus_t, callbackClientGroupStatus);
		STEAM_GAMESERVER_CALLBACK(SteamServer, associate_clan, AssociateWithClanResult_t, callbackAssociateClan);
		STEAM_GAMESERVER_CALLBACK(SteamServer, player_compat, ComputeNewPlayerCompatibilityResult_t, callbackPlayerCompat);

		// Game Server Stat callbacks ///////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, stats_stored, GSStatsStored_t, callbackStatsStored);
		STEAM_GAMESERVER_CALLBACK(SteamServer, stats_unloaded, GSStatsUnloaded_t, callbackStatsUnloaded);

		// HTTP callbacks ///////////////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, http_request_completed, HTTPRequestCompleted_t, callbackHTTPRequestCompleted);
		STEAM_GAMESERVER_CALLBACK(SteamServer, http_request_data_received, HTTPRequestDataReceived_t, callbackHTTPRequestDataReceived);
		STEAM_GAMESERVER_CALLBACK(SteamServer, http_request_headers_received, HTTPRequestHeadersReceived_t, callbackHTTPRequestHeadersReceived);

		// Inventory callbacks //////////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, inventory_definition_update, SteamInventoryDefinitionUpdate_t, callbackInventoryDefinitionUpdate);
		STEAM_GAMESERVER_CALLBACK(SteamServer, inventory_full_update, SteamInventoryFullUpdate_t, callbackInventoryFullUpdate);
		STEAM_GAMESERVER_CALLBACK(SteamServer, inventory_result_ready, SteamInventoryResultReady_t, callbackInventoryResultReady);

		// Networking callbacks /////////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, p2p_session_connect_fail, P2PSessionConnectFail_t, callbackP2PSessionConnectFail);
		STEAM_GAMESERVER_CALLBACK(SteamServer, p2p_session_request, P2PSessionRequest_t, callbackP2PSessionRequest);

		// Networking Messages callbacks ////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, network_messages_session_request, SteamNetworkingMessagesSessionRequest_t, callbackNetworkMessagesSessionRequest);
		STEAM_GAMESERVER_CALLBACK(SteamServer, network_messages_session_failed, SteamNetworkingMessagesSessionFailed_t, callbackNetworkMessagesSessionFailed);

		// Networking Sockets callbacks /////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, network_connection_status_changed, SteamNetConnectionStatusChangedCallback_t, callbackNetworkConnectionStatusChanged);
		STEAM_GAMESERVER_CALLBACK(SteamServer, network_authentication_status, SteamNetAuthenticationStatus_t, callbackNetworkAuthenticationStatus);
		STEAM_GAMESERVER_CALLBACK(SteamServer, fake_ip_result, SteamNetworkingFakeIPResult_t, callbackNetworkingFakeIPResult);

		// Networking Utils callbacks ///////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, relay_network_status, SteamRelayNetworkStatus_t, callbackRelayNetworkStatus);

		// Remote Storage callbacks /////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, local_file_changed, RemoteStorageLocalFileChange_t, callbackLocalFileChanged);

		// UGC callbacks ////////////////////////
		STEAM_GAMESERVER_CALLBACK(SteamServer, item_downloaded, DownloadItemResult_t, callbackItemDownloaded);
		STEAM_GAMESERVER_CALLBACK(SteamServer, item_installed, ItemInstalled_t, callbackItemInstalled);
		STEAM_GAMESERVER_CALLBACK(SteamServer, user_subscribed_items_list_changed, UserSubscribedItemsListChanged_t, callbackUserSubscribedItemsListChanged);


		/////////////////////////////////////////
		// STEAM CALL RESULTS ///////////////////
		/////////////////////////////////////////
		//
		// Game Server Stats call results ///////
		CCallResult<SteamServer, GSStatsReceived_t> callResultStatReceived;
		void stats_received(GSStatsReceived_t *call_data, bool io_failure);

		// Inventory call results ///////////////
		CCallResult<SteamServer, SteamInventoryEligiblePromoItemDefIDs_t> callResultEligiblePromoItemDefIDs;
		void inventory_eligible_promo_item(SteamInventoryEligiblePromoItemDefIDs_t *call_data, bool io_failure);
		CCallResult<SteamServer, SteamInventoryRequestPricesResult_t> callResultRequestPrices;
		void inventory_request_prices_result(SteamInventoryRequestPricesResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, SteamInventoryStartPurchaseResult_t> callResultStartPurchase;
		void inventory_start_purchase_result(SteamInventoryStartPurchaseResult_t *call_data, bool io_failure);

		// Remote Storage call results //////////
		CCallResult<SteamServer, RemoteStorageFileReadAsyncComplete_t> callResultFileReadAsyncComplete;
		void file_read_async_complete(RemoteStorageFileReadAsyncComplete_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoteStorageFileShareResult_t> callResultFileShareResult;
		void file_share_result(RemoteStorageFileShareResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoteStorageFileWriteAsyncComplete_t> callResultFileWriteAsyncComplete;
		void file_write_async_complete(RemoteStorageFileWriteAsyncComplete_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoteStorageDownloadUGCResult_t> callResultDownloadUGCResult;
		void download_ugc_result(RemoteStorageDownloadUGCResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoteStorageUnsubscribePublishedFileResult_t> callResultUnsubscribeItem;
		void unsubscribe_item(RemoteStorageUnsubscribePublishedFileResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoteStorageSubscribePublishedFileResult_t> callResultSubscribeItem;
		void subscribe_item(RemoteStorageSubscribePublishedFileResult_t *call_data, bool io_failure);

		// UGC call results /////////////////////
		CCallResult<SteamServer, AddAppDependencyResult_t> callResultAddAppDependency;
		void add_app_dependency_result(AddAppDependencyResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, AddUGCDependencyResult_t> callResultAddUGCDependency;
		void add_ugc_dependency_result(AddUGCDependencyResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, CreateItemResult_t> callResultItemCreate;
		void item_created(CreateItemResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, GetAppDependenciesResult_t> callResultGetAppDependencies;
		void get_app_dependencies_result(GetAppDependenciesResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, DeleteItemResult_t> callResultDeleteItem;
		void item_deleted(DeleteItemResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, GetUserItemVoteResult_t> callResultGetUserItemVote;
		void get_item_vote_result(GetUserItemVoteResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoveAppDependencyResult_t> callResultRemoveAppDependency;
		void remove_app_dependency_result(RemoveAppDependencyResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, RemoveUGCDependencyResult_t> callResultRemoveUGCDependency;
		void remove_ugc_dependency_result(RemoveUGCDependencyResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, SetUserItemVoteResult_t> callResultSetUserItemVote;
		void set_user_item_vote(SetUserItemVoteResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, StartPlaytimeTrackingResult_t> callResultStartPlaytimeTracking;
		void start_playtime_tracking(StartPlaytimeTrackingResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, SteamUGCQueryCompleted_t> callResultUGCQueryCompleted;
		void ugc_query_completed(SteamUGCQueryCompleted_t *call_data, bool io_failure);
		CCallResult<SteamServer, StopPlaytimeTrackingResult_t> callResultStopPlaytimeTracking;
		void stop_playtime_tracking(StopPlaytimeTrackingResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, SubmitItemUpdateResult_t> callResultItemUpdate;
		void item_updated(SubmitItemUpdateResult_t *call_data, bool io_failure);
		CCallResult<SteamServer, UserFavoriteItemsListChanged_t> callResultFavoriteItemListChanged;
		void user_favorite_items_list_changed(UserFavoriteItemsListChanged_t *call_data, bool io_failure);
		CCallResult<SteamServer, WorkshopEULAStatus_t> callResultWorkshopEULAStatus;
		void workshop_eula_status(WorkshopEULAStatus_t *call_data, bool io_failure);
	};
}
#endif // GODOTSTEAM_SERVER_H