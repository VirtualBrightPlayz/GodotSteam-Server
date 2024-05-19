extends Node

# Steam Server variables
onready var SteamServer = preload("res://addons/godotsteam_server/godotsteam_server.gdns").new()

var is_on_steam: bool = false
var is_server_secure: bool = false
var server_game_port: int = 27015
var server_initialize: Dictionary = {}
var server_ip_address: String = ""
var server_query_port: int = 27015
var server_steam_id: int = 0
var server_version: String = ""


func _ready() -> void:
	# Initialize Steam Server
	server_initialize = SteamServer.steamInitEx(
		server_ip_address,
		server_game_port,
		server_query_port,
		1,
		server_version)

	print("[STEAM] Did Steam Server initialize?: %s" % server_initialize)
	if server_initialize['status'] > 0:
		# If Steam fails to start up, shut down the app
		print("[STEAM] %s Shutting down..." % server_initialize['verbal'])
#		get_tree().quit()
	
	server_steam_id = SteamServer.getServerSteamID()
	is_server_secure = SteamServer.isServerSecure()


func _process(_delta: float) -> void:
	# Get callbacks
	SteamServer.run_callbacks()


###
# You can add more functionality here and just call it through GDScript like so: 
#	Steam._set_Achievements( achievement_name )
#
# The corresponding function in this file would be something like:
#	func _set_Achievement(this_achievement: String) -> void:
#		print("Setting Steam achievement: "+str(this_achievement))
#		var WAS_SET: bool = Steam.setAchievement(this_achievement)
#		print("Steam achievement "+str(this_achievement)+" set: "+str(WAS_SET))
#		Steam.storeStats()
#
# Due to how GDNative works, the functions must either reside here or be called with Steam.Steam. since we are calling this singleton script then the Steamworks class.
###