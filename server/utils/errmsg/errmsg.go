package errmsg

const (
	SUCCESS = 0
	ERROR   = 500

	ErrorIntern         = 1000
	ErrorData           = 1001
	ErrorTokenOvertime  = 1002
	ErrorTokenWrong     = 1003
	ErrorWrongTokenType = 1004
)

var errMap = map[int]string{
	SUCCESS: "OK",
	ERROR:   "fail",

	ErrorIntern:         "something went wrong",
	ErrorData:           "invalid data",
	ErrorTokenOvertime:  "token overtime",
	ErrorTokenWrong:     "wrong token",
	ErrorWrongTokenType: "wrong token type",
}

func GetErrMsg(code int) string {
	return errMap[code]
}
