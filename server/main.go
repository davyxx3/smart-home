package main

import (
	"smart-home/model"
	"smart-home/routes"
)

func main() {
	model.InitDb()
	routes.InitRouter()
}
