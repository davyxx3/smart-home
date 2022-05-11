package main

import (
	"smart-home/core"
	"smart-home/sql"
)

func main() {
	// 初始化数据库
	sql.InitDb()
	// 启动服务器
	core.RunServer()
}
