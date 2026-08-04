
### <u>Battery calculation</u>
power required for 4 hours of use

#### Battery -> Buck
```math
volt3 = 3.3V
volt5 = 5V

#current
stm32 = 250mA
gps = 70mA
lora = 110mA
crypt = 50mA

power = (volt5 * lora) + (volt3 * (stm32 + gps + crypt))

#required watt hours
operating_time = 4hr

energy = power * operating_time to Wh

#required battery mah
li_typ_volt = 3.7V

mah = round((energy / li_typ_volt) to mAh, mAh) to mAh
peak_current = power / 3.7V



```

### Battery -> Buck -> LDO
```math
volt5 = 5V

#current
stm32 = 250mA
gps = 70mA
lora = 110mA
crypt = 50mA

total_current = stm32 + gps + lora + crypt

power = volt5 * total_current in W

#required energy
operating_time = 4hr

energy = power * operating_time in Wh

#required Battery
li_typ_volt = 3.7V

mah = round((energy / li_typ_volt) to mAh, mAh) to mAh
peak_current = power / 3.7V


```

