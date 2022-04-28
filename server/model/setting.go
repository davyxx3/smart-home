package model

import (
	"gopkg.in/ini.v1"
	"smart-home/middleware"
)

type Configuration struct {
	AppMode  string
	HttpPort string

	Db         string
	DbHost     string
	DbPort     string
	DbUser     string
	DbPassword string
	DbName     string
}

var Settings = Configuration{}

func init() {
	file, err := ini.Load("config/config.ini")
	if err != nil {
		middleware.Logger.Fatal("can not find the configuration file")
	}

	loadServerConfig(file)
	loadDatabaseConfig(file)
}

func loadServerConfig(file *ini.File) {
	Settings.AppMode = file.Section("server").Key("AppMode").MustString("debug")
	Settings.HttpPort = file.Section("server").Key("HttpPort").MustString("3000")
}

func loadDatabaseConfig(file *ini.File) {
	Settings.Db = file.Section("database").Key("Db").MustString("mysql")
	Settings.DbHost = file.Section("database").Key("Dbhost").MustString("localhost")
	Settings.DbPort = file.Section("database").Key("DbPort").MustString("3306")
	Settings.DbUser = file.Section("database").Key("DbUser").MustString("root")
	Settings.DbPassword = file.Section("database").Key("DbPassword").MustString("bhqhz0209")
	Settings.DbName = file.Section("database").Key("DbName").MustString("smart_home")
}
