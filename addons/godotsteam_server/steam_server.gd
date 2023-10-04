extends Node

# Steam Server variables
onready var SteamServer = preload("res://addons/godotsteam_server/godotsteam_server.gdns").new()


func _ready() -> void:
	# Initialize Steam Server
	var INIT: Dictionary = SteamServer.steamInitEx(false)
	print("[STEAM] Did Steam Server initialize?: "+str(INIT))
	if INIT['status'] != 1:
		# If Steam fails to start up, shut down the app
		print("[STEAM] Failed to initialize Steam Server. "+str(INIT['verbal'])+" Shutting down...")
#		get_tree().quit()


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