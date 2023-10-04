tool
extends EditorPlugin

func _enter_tree() -> void:
	add_autoload_singleton("SteamServer", "res://addons/godotsteam_server/steam_server.gd")


func _exit_tree() -> void:
	remove_autoload_singleton("SteamServer")
