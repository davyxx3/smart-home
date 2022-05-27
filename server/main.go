package main

import (
	"smart-home/core"
	"smart-home/sql"
)

// @Smart Home 家居环境采集和监测系统
// @version 1.0
// @description 这里写描述信息
// @termsOfService http://swagger.io/terms/

// @contact.name 这里写联系人信息
// @contact.url http://www.swagger.io/support
// @contact.email support@swagger.io

// @license.name Apache 2.0
// @license.url http://www.apache.org/licenses/LICENSE-2.0.html

// @host 3000
// @BasePath /
func main() {
	// 初始化数据库
	sql.InitDb()
	// 启动服务器
	core.RunServer()
}
