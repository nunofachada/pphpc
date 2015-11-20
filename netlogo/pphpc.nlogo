globals [
  sheep-count     ;; how many sheep are there?
  wolves-count    ;; how many wolves are there?
  grass-alive     ;; how much alive grass there is
  sheep-energy    ;; mean sheep energy
  wolves-energy   ;; mean wolf energy
  grass-countdown ;; mean grass countdown
  maxwho          ;; youngest agent to date
]

;; Sheep and wolves are both breeds of turtle.
breed [sheep a-sheep]   ;; sheep is its own plural, so we use "a-sheep" as the singular.
breed [wolves wolf]
turtles-own [energy]    ;; both wolves and sheep have energy
patches-own [countdown] ;; patches have a countdown variable for grass

;; Initialization
to setup

  clear-all
  
  ;; Set random number generator seed
  random-seed rngseed
  
  ;; show-params

  ask patches [ 
    ifelse random 2 = 1 [
      set countdown 1 + random grass-regrowth-time ;; initialize grass grow clocks randomly
      set pcolor brown
    ] [
      set countdown 0
      set pcolor green 
    ]
  ]

  set-default-shape sheep "sheep"
  create-sheep initial-number-sheep  ;; create the sheep, then initialize their variables
  [
    set color white
    set size 1  ;; easier to see
    set label-color blue - 2
    set energy 1 + random (2 * sheep-gain-from-food)
    setxy random-pxcor random-pycor
  ]
  set-default-shape wolves "wolf"
  create-wolves initial-number-wolves  ;; create the wolves, then initialize their variables
  [
    set color black
    set size 1  ;; easier to see
    set energy 1 + random (2 * wolf-gain-from-food)
    setxy random-pxcor random-pycor
  ]

  gather-stats

  display-labels
  reset-ticks

end

;; Simulation loop
to go

  ;; 1 - Agent movement
  ask turtles [
    move
    set energy energy - 1
    if energy < 1 [ die ] ;; if energy dips below zero, die
  ]
  
  ;; 2 - Grow food
  ask patches [ grow-grass ]
  
  ;; 3 - Act
  ask turtles [
    ifelse is-a-sheep? self [
      ;; is a sheep
      eat-grass
      reproduce sheep-reprod-thres sheep-reprod-prob
    ] [ 
      ;; is a wolf
      catch-sheep
      reproduce wolf-reprod-thres wolf-reprod-prob
    ]
  ]

  ;; 4 - Gather stats
  gather-stats
  display-labels
  
  ;; New iteration
  tick
  
  ;; Time to stop?
  if ticks = iterations [ stop ]
  
end

to gather-stats
  set maxwho max [who] of sheep
  set sheep-count count sheep
  set wolves-count count wolves
  set grass-alive count patches with [countdown <= 0]
  if show-energy? [
    ifelse sheep-count > 0
      [ set sheep-energy mean [energy] of sheep ]
      [ set sheep-energy 0 ]
    ifelse wolves-count > 0
      [ set wolves-energy mean [energy] of wolves ]
      [ set wolves-energy 0 ]
    set grass-countdown mean [countdown] of patches
  ]
end

to move  ;; turtle procedure
  let direction random 5
  if direction < 4 [
    set heading 90 * direction
    fd 1
  ]
end

to eat-grass  ;; sheep procedure
  ;; sheep eat grass, turn the patch brown
  if pcolor = green [
    set pcolor brown
    ask patch-here [set countdown grass-regrowth-time]
    set energy energy + sheep-gain-from-food  ;; sheep gain energy by eating
  ]
end

to reproduce [ reprod-thres reprod-prob ] ;; turtle procedure
  if energy > reprod-thres [
    if random 100 < reprod-prob [  ;; throw "dice" to see if you will reproduce
      let energy_offspring int (energy / 2)
      set energy energy - energy_offspring    ;; divide energy between parent and offspring
      hatch 1 [ ;; hatch an offspring which stays in the same place
        set energy energy_offspring  ] 
    ]
  ]
end

to catch-sheep  ;; wolf procedure
  let prey one-of sheep-here with [ who <= maxwho ]  ;; grab a random sheep, cannot be a newly born sheep
  if prey != nobody                             ;; did we get one?  if so,
    [ ask prey [ die ]                          ;; kill it
      set energy energy + wolf-gain-from-food ] ;; get energy from eating
end

to grow-grass  ;; patch procedure
  ;; countdown on brown patches: if reach 0, grow some grass
  if pcolor = brown [
    set countdown countdown - 1
    if countdown <= 0
      [ set pcolor green
        set countdown 0 ]
  ]
end

to display-labels
  if show-energy? [
    ask wolves [ set label energy ]
    ask sheep [ set label energy ]
  ]
end

to show-params
  print word "                INIT_SHEEP = " initial-number-sheep
  print word "      SHEEP_GAIN_FROM_FOOD = " sheep-gain-from-food
  print word " SHEEP_REPRODUCE_THRESHOLD = " sheep-reprod-thres
  print word "      SHEEP_REPRODUCE_PROB = " sheep-reprod-prob
  print word "               INIT_WOLVES = " initial-number-wolves
  print word "     WOLVES_GAIN_FROM_FOOD = " wolf-gain-from-food
  print word "WOLVES_REPRODUCE_THRESHOLD = " wolf-reprod-thres
  print word "     WOLVES_REPRODUCE_PROB = " wolf-reprod-prob
  print word "             GRASS_RESTART = " grass-regrowth-time
  print word "                    GRID_X = " world-width
  print word "                    GRID_Y = " world-height
  print word "                     ITERS = " iterations
end

; Copyright 1997 Uri Wilensky. All rights reserved.
; Copyright 2015 Nuno Fachada. All rights reserved.
; The full copyright notice is in the Information tab.
@#$#@#$#@
GRAPHICS-WINDOW
646
10
1056
441
-1
-1
4.0
1
9
1
1
1
0
1
1
1
0
99
0
99
1
1
1
ticks
30.0

SLIDER
9
248
101
281
sheep-gain-from-food
sheep-gain-from-food
0.0
50.0
4
1.0
1
NIL
HORIZONTAL

SLIDER
9
283
101
316
sheep-reprod-prob
sheep-reprod-prob
0
20.0
4
1.0
1
%
HORIZONTAL

SLIDER
103
248
196
281
wolf-gain-from-food
wolf-gain-from-food
0.0
100.0
20
1.0
1
NIL
HORIZONTAL

SLIDER
103
283
196
316
wolf-reprod-prob
wolf-reprod-prob
0.0
20.0
5
1.0
1
%
HORIZONTAL

SLIDER
12
59
104
92
grass-regrowth-time
grass-regrowth-time
0
100
10
1
1
NIL
HORIZONTAL

BUTTON
22
10
77
43
setup
setup
NIL
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

BUTTON
78
10
133
43
go
go
T
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

PLOT
202
10
569
263
populations
time
pop.
0.0
100.0
0.0
100.0
true
true
"" ""
PENS
"sheep" 1.0 0 -13345367 true "" "plot sheep-count"
"wolves" 1.0 0 -2674135 true "" "plot wolves-count"
"grass / 4" 1.0 0 -10899396 true "" "plot grass-alive / 4"

MONITOR
571
10
642
55
sheep
sheep-count
3
1
11

MONITOR
571
58
642
103
wolves
wolves-count
3
1
11

MONITOR
571
106
642
151
grass
grass-alive
0
1
11

TEXTBOX
15
172
155
191
Sheep settings
11
0.0
0

TEXTBOX
114
173
227
191
Wolf settings
11
0.0
1

TEXTBOX
16
46
168
64
Grass settings
11
0.0
1

SWITCH
106
58
196
91
show-energy?
show-energy?
1
1
-1000

INPUTBOX
8
318
101
378
sheep-reprod-thres
2
1
0
Number

INPUTBOX
103
318
196
378
wolf-reprod-thres
2
1
0
Number

INPUTBOX
9
186
101
246
initial-number-sheep
400
1
0
Number

INPUTBOX
103
186
196
246
initial-number-wolves
200
1
0
Number

INPUTBOX
64
109
153
169
iterations
4001
1
0
Number

TEXTBOX
48
95
198
113
Number of iterations
11
0.0
1

BUTTON
103
379
158
412
Reset
set iterations 2000\nset show-energy? false\nset grass-regrowth-time 10\nset initial-number-sheep 400\nset sheep-gain-from-food 4\nset sheep-reprod-prob 4\nset sheep-reprod-thres 2\nset initial-number-wolves 200\nset wolf-gain-from-food 20\nset wolf-reprod-prob 5\nset wolf-reprod-thres 2\nset rngseed new-seed\n
NIL
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

PLOT
202
264
569
511
energy
time
energy
0.0
100.0
0.0
50.0
true
true
"" "if show-energy? [\n  set-current-plot-pen \"sheep\"\n  plot sheep-energy\n  set-current-plot-pen \"wolves\"\n  plot wolves-energy\n  set-current-plot-pen \"grass * 4\"\n  plot grass-countdown * 4\n]"
PENS
"sheep" 1.0 0 -13345367 true "" ""
"wolves" 1.0 0 -2674135 true "" ""
"grass * 4" 1.0 0 -10899396 true "" ""

TEXTBOX
114
46
264
64
Show energy?
11
0.0
1

MONITOR
570
264
642
309
sheep
sheep-energy
3
1
11

MONITOR
570
313
642
358
wolves
wolves-energy
3
1
11

MONITOR
571
360
642
405
grass
grass-countdown
3
1
11

BUTTON
134
10
193
43
step
go
NIL
1
T
OBSERVER
NIL
NIL
NIL
NIL
1

INPUTBOX
8
380
101
440
rngseed
1784790635
1
0
Number

@#$#@#$#@
## WHAT IS IT?

The purpose of PPHPC is to serve as a standard model for studying and evaluating spatial agent-based model (SABM) implementation strategies. It is a realization of a predator-prey dynamic system, and captures important characteristics of SABMs, such as agent movement and local agent interactions. The model can be implemented using substantially different approaches that ensure statistically equivalent qualitative results. Implementations may differ in aspects such as the selected system architecture, choice of programming language and/or agent-based modeling framework, parallelization strategy, random number generator, and so forth. By comparing distinct PPHPC implementations, valuable insights can be obtained on the computational and algorithmical design of SABMs in general.

The NetLogo implementation of PPHPC is based on NetLogo's own "Wolf Sheep Predation" model by Uri Wilensky (see ref. 2), considerably modified to follow the model's ODD protocol described in ref. 1.

## REFERENCES

1. Fachada N, Lopes VV, Martins RC, Rosa AC. (2015) Towards a standard model for research in agent-based modeling and simulation. *PeerJ Computer Science* 1:e36 https://dx.doi.org/10.7717/peerj-cs.36

2. Wilensky, U. (1997).  NetLogo Wolf Sheep Predation model.  http://ccl.northwestern.edu/netlogo/models/WolfSheepPredation.  Center for Connected Learning and Computer-Based Modeling, Northwestern University, Evanston, IL.  

## COPYRIGHT NOTICES

### For this model

This model is made available under the CC BY-NC-SA 4.0 license which allows you to:

* Share — copy and redistribute the material in any medium or format
* Adapt — remix, transform, and build upon the material

Under the following terms:

* Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
* NonCommercial — You may not use the material for commercial purposes.
* ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.

The legal text for this license is available at
http://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

### For the original model

Copyright 1997 Uri Wilensky. All rights reserved.

Permission to use, modify or redistribute this model is hereby granted, provided that both of the following requirements are followed:  
a) this copyright notice is included.  
b) this model will not be redistributed for profit without permission from Uri Wilensky. Contact Uri Wilensky for appropriate licenses for redistribution for profit.
@#$#@#$#@
default
true
0
Polygon -7500403 true true 150 5 40 250 150 205 260 250

airplane
true
0
Polygon -7500403 true true 150 0 135 15 120 60 120 105 15 165 15 195 120 180 135 240 105 270 120 285 150 270 180 285 210 270 165 240 180 180 285 195 285 165 180 105 180 60 165 15

arrow
true
0
Polygon -7500403 true true 150 0 0 150 105 150 105 293 195 293 195 150 300 150

box
false
0
Polygon -7500403 true true 150 285 285 225 285 75 150 135
Polygon -7500403 true true 150 135 15 75 150 15 285 75
Polygon -7500403 true true 15 75 15 225 150 285 150 135
Line -16777216 false 150 285 150 135
Line -16777216 false 150 135 15 75
Line -16777216 false 150 135 285 75

bug
true
0
Circle -7500403 true true 96 182 108
Circle -7500403 true true 110 127 80
Circle -7500403 true true 110 75 80
Line -7500403 true 150 100 80 30
Line -7500403 true 150 100 220 30

butterfly
true
0
Polygon -7500403 true true 150 165 209 199 225 225 225 255 195 270 165 255 150 240
Polygon -7500403 true true 150 165 89 198 75 225 75 255 105 270 135 255 150 240
Polygon -7500403 true true 139 148 100 105 55 90 25 90 10 105 10 135 25 180 40 195 85 194 139 163
Polygon -7500403 true true 162 150 200 105 245 90 275 90 290 105 290 135 275 180 260 195 215 195 162 165
Polygon -16777216 true false 150 255 135 225 120 150 135 120 150 105 165 120 180 150 165 225
Circle -16777216 true false 135 90 30
Line -16777216 false 150 105 195 60
Line -16777216 false 150 105 105 60

car
false
0
Polygon -7500403 true true 300 180 279 164 261 144 240 135 226 132 213 106 203 84 185 63 159 50 135 50 75 60 0 150 0 165 0 225 300 225 300 180
Circle -16777216 true false 180 180 90
Circle -16777216 true false 30 180 90
Polygon -16777216 true false 162 80 132 78 134 135 209 135 194 105 189 96 180 89
Circle -7500403 true true 47 195 58
Circle -7500403 true true 195 195 58

circle
false
0
Circle -7500403 true true 0 0 300

circle 2
false
0
Circle -7500403 true true 0 0 300
Circle -16777216 true false 30 30 240

cow
false
0
Polygon -7500403 true true 200 193 197 249 179 249 177 196 166 187 140 189 93 191 78 179 72 211 49 209 48 181 37 149 25 120 25 89 45 72 103 84 179 75 198 76 252 64 272 81 293 103 285 121 255 121 242 118 224 167
Polygon -7500403 true true 73 210 86 251 62 249 48 208
Polygon -7500403 true true 25 114 16 195 9 204 23 213 25 200 39 123

cylinder
false
0
Circle -7500403 true true 0 0 300

dot
false
0
Circle -7500403 true true 90 90 120

face happy
false
0
Circle -7500403 true true 8 8 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Polygon -16777216 true false 150 255 90 239 62 213 47 191 67 179 90 203 109 218 150 225 192 218 210 203 227 181 251 194 236 217 212 240

face neutral
false
0
Circle -7500403 true true 8 7 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Rectangle -16777216 true false 60 195 240 225

face sad
false
0
Circle -7500403 true true 8 8 285
Circle -16777216 true false 60 75 60
Circle -16777216 true false 180 75 60
Polygon -16777216 true false 150 168 90 184 62 210 47 232 67 244 90 220 109 205 150 198 192 205 210 220 227 242 251 229 236 206 212 183

fish
false
0
Polygon -1 true false 44 131 21 87 15 86 0 120 15 150 0 180 13 214 20 212 45 166
Polygon -1 true false 135 195 119 235 95 218 76 210 46 204 60 165
Polygon -1 true false 75 45 83 77 71 103 86 114 166 78 135 60
Polygon -7500403 true true 30 136 151 77 226 81 280 119 292 146 292 160 287 170 270 195 195 210 151 212 30 166
Circle -16777216 true false 215 106 30

flag
false
0
Rectangle -7500403 true true 60 15 75 300
Polygon -7500403 true true 90 150 270 90 90 30
Line -7500403 true 75 135 90 135
Line -7500403 true 75 45 90 45

flower
false
0
Polygon -10899396 true false 135 120 165 165 180 210 180 240 150 300 165 300 195 240 195 195 165 135
Circle -7500403 true true 85 132 38
Circle -7500403 true true 130 147 38
Circle -7500403 true true 192 85 38
Circle -7500403 true true 85 40 38
Circle -7500403 true true 177 40 38
Circle -7500403 true true 177 132 38
Circle -7500403 true true 70 85 38
Circle -7500403 true true 130 25 38
Circle -7500403 true true 96 51 108
Circle -16777216 true false 113 68 74
Polygon -10899396 true false 189 233 219 188 249 173 279 188 234 218
Polygon -10899396 true false 180 255 150 210 105 210 75 240 135 240

house
false
0
Rectangle -7500403 true true 45 120 255 285
Rectangle -16777216 true false 120 210 180 285
Polygon -7500403 true true 15 120 150 15 285 120
Line -16777216 false 30 120 270 120

leaf
false
0
Polygon -7500403 true true 150 210 135 195 120 210 60 210 30 195 60 180 60 165 15 135 30 120 15 105 40 104 45 90 60 90 90 105 105 120 120 120 105 60 120 60 135 30 150 15 165 30 180 60 195 60 180 120 195 120 210 105 240 90 255 90 263 104 285 105 270 120 285 135 240 165 240 180 270 195 240 210 180 210 165 195
Polygon -7500403 true true 135 195 135 240 120 255 105 255 105 285 135 285 165 240 165 195

line
true
0
Line -7500403 true 150 0 150 300

line half
true
0
Line -7500403 true 150 0 150 150

pentagon
false
0
Polygon -7500403 true true 150 15 15 120 60 285 240 285 285 120

person
false
0
Circle -7500403 true true 110 5 80
Polygon -7500403 true true 105 90 120 195 90 285 105 300 135 300 150 225 165 300 195 300 210 285 180 195 195 90
Rectangle -7500403 true true 127 79 172 94
Polygon -7500403 true true 195 90 240 150 225 180 165 105
Polygon -7500403 true true 105 90 60 150 75 180 135 105

plant
false
0
Rectangle -7500403 true true 135 90 165 300
Polygon -7500403 true true 135 255 90 210 45 195 75 255 135 285
Polygon -7500403 true true 165 255 210 210 255 195 225 255 165 285
Polygon -7500403 true true 135 180 90 135 45 120 75 180 135 210
Polygon -7500403 true true 165 180 165 210 225 180 255 120 210 135
Polygon -7500403 true true 135 105 90 60 45 45 75 105 135 135
Polygon -7500403 true true 165 105 165 135 225 105 255 45 210 60
Polygon -7500403 true true 135 90 120 45 150 15 180 45 165 90

sheep
false
15
Rectangle -1 true true 166 225 195 285
Rectangle -1 true true 62 225 90 285
Rectangle -1 true true 30 75 210 225
Circle -1 true true 135 75 150
Circle -7500403 true false 180 76 116

square
false
0
Rectangle -7500403 true true 30 30 270 270

square 2
false
0
Rectangle -7500403 true true 30 30 270 270
Rectangle -16777216 true false 60 60 240 240

star
false
0
Polygon -7500403 true true 151 1 185 108 298 108 207 175 242 282 151 216 59 282 94 175 3 108 116 108

target
false
0
Circle -7500403 true true 0 0 300
Circle -16777216 true false 30 30 240
Circle -7500403 true true 60 60 180
Circle -16777216 true false 90 90 120
Circle -7500403 true true 120 120 60

tree
false
0
Circle -7500403 true true 118 3 94
Rectangle -6459832 true false 120 195 180 300
Circle -7500403 true true 65 21 108
Circle -7500403 true true 116 41 127
Circle -7500403 true true 45 90 120
Circle -7500403 true true 104 74 152

triangle
false
0
Polygon -7500403 true true 150 30 15 255 285 255

triangle 2
false
0
Polygon -7500403 true true 150 30 15 255 285 255
Polygon -16777216 true false 151 99 225 223 75 224

truck
false
0
Rectangle -7500403 true true 4 45 195 187
Polygon -7500403 true true 296 193 296 150 259 134 244 104 208 104 207 194
Rectangle -1 true false 195 60 195 105
Polygon -16777216 true false 238 112 252 141 219 141 218 112
Circle -16777216 true false 234 174 42
Rectangle -7500403 true true 181 185 214 194
Circle -16777216 true false 144 174 42
Circle -16777216 true false 24 174 42
Circle -7500403 false true 24 174 42
Circle -7500403 false true 144 174 42
Circle -7500403 false true 234 174 42

turtle
true
0
Polygon -10899396 true false 215 204 240 233 246 254 228 266 215 252 193 210
Polygon -10899396 true false 195 90 225 75 245 75 260 89 269 108 261 124 240 105 225 105 210 105
Polygon -10899396 true false 105 90 75 75 55 75 40 89 31 108 39 124 60 105 75 105 90 105
Polygon -10899396 true false 132 85 134 64 107 51 108 17 150 2 192 18 192 52 169 65 172 87
Polygon -10899396 true false 85 204 60 233 54 254 72 266 85 252 107 210
Polygon -7500403 true true 119 75 179 75 209 101 224 135 220 225 175 261 128 261 81 224 74 135 88 99

wheel
false
0
Circle -7500403 true true 3 3 294
Circle -16777216 true false 30 30 240
Line -7500403 true 150 285 150 15
Line -7500403 true 15 150 285 150
Circle -7500403 true true 120 120 60
Line -7500403 true 216 40 79 269
Line -7500403 true 40 84 269 221
Line -7500403 true 40 216 269 79
Line -7500403 true 84 40 221 269

wolf
false
0
Rectangle -7500403 true true 195 106 285 150
Rectangle -7500403 true true 195 90 255 105
Polygon -7500403 true true 240 90 217 44 196 90
Polygon -16777216 true false 234 89 218 59 203 89
Rectangle -1 true false 240 93 252 105
Rectangle -16777216 true false 242 96 249 104
Rectangle -16777216 true false 241 125 285 139
Polygon -1 true false 285 125 277 138 269 125
Polygon -1 true false 269 140 262 125 256 140
Rectangle -7500403 true true 45 120 195 195
Rectangle -7500403 true true 45 114 185 120
Rectangle -7500403 true true 165 195 180 270
Rectangle -7500403 true true 60 195 75 270
Polygon -7500403 true true 45 105 15 30 15 75 45 150 60 120

x
false
0
Polygon -7500403 true true 270 75 225 30 30 225 75 270
Polygon -7500403 true true 30 75 75 30 270 225 225 270

@#$#@#$#@
NetLogo 5.1.0
@#$#@#$#@
setup
set grass? true
repeat 75 [ go ]
@#$#@#$#@
@#$#@#$#@
<experiments>
  <experiment name="ex100v1" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="99"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="99"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="20"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex200v1" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="1600"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="199"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="199"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="800"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="20"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex400v1" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="6400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="399"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="399"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="3200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="20"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex800v1" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="25600"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="799"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="799"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="12800"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="20"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex1600v1" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="102400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="4"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="1599"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="1599"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="51200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="20"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex100v2" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="30"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="99"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="99"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="15"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex200v2" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="1600"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="30"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="199"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="199"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="800"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="15"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex400v2" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="6400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="30"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="399"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="399"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="3200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="15"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex800v2" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="25600"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="30"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="799"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="799"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="12800"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="15"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
  <experiment name="ex1600v2" repetitions="1" runMetricsEveryStep="true">
    <setup>setup</setup>
    <go>go</go>
    <metric>sheep-count</metric>
    <metric>wolves-count</metric>
    <metric>grass-alive</metric>
    <metric>sheep-energy</metric>
    <metric>wolves-energy</metric>
    <metric>grass-countdown</metric>
    <enumeratedValueSet variable="initial-number-sheep">
      <value value="102400"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-gain-from-food">
      <value value="30"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-prob">
      <value value="5"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-prob">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="sheep-reprod-thres">
      <value value="2"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pxcor">
      <value value="1599"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pxcor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="max-pycor">
      <value value="1599"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="min-pycor">
      <value value="0"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="initial-number-wolves">
      <value value="51200"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="grass-regrowth-time">
      <value value="15"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="wolf-gain-from-food">
      <value value="10"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="iterations">
      <value value="4001"/>
    </enumeratedValueSet>
    <enumeratedValueSet variable="show-energy?">
      <value value="true"/>
    </enumeratedValueSet>
  </experiment>
</experiments>
@#$#@#$#@
@#$#@#$#@
default
0.0
-0.2 0 0.0 1.0
0.0 1 1.0 0.0
0.2 0 0.0 1.0
link direction
true
0
Line -7500403 true 150 150 90 180
Line -7500403 true 150 150 210 180

@#$#@#$#@
0
@#$#@#$#@
