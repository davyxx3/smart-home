package core

import (
	"smart-home/middleware"
	"smart-home/routes"
	"smart-home/sql"
)

func RunServer() {
	r := routes.Init()
	middleware.Logger.Error(r.Run(sql.Settings.HttpPort).Error())
}
