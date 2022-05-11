package model

type RawData struct {
	DevId   int     `gorm:"type: int;not null" json:"dev_id"`
	DevName string  `gorm:"type: varchar(20)" json:"dev_name"`
	DevVol  float64 `gorm:"type: double" json:"dev_vol"`
	DevBat  float64 `gorm:"type: double" json:"dev_bat"`
	DevAng  float64 `gorm:"type: double" json:"dev_ang"`
	DevSig  float64 `gorm:"type: double" json:"dev_sig"`
	Time    string  `gorm:"type: datetime" json:"time"`
	Tem     float64 `gorm:"type: double" json:"tem"`
	Hum     float64 `gorm:"type: double" json:"hum"`
	CoCon   float64 `gorm:"type: double" json:"coCon"`
	PmCon   float64 `gorm:"type: double" json:"pmCon"`
	Smk     float64 `gorm:"type: double" json:"smk"`
	Alarm   int     `gorm:"type: int" json:"alarm"`
}
