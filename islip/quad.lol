BTW Example script for solving quadratic equations; makes use of most, if not all language constructs in LOLCODE

HAI 1.2
CAN HAS STDIO?

VISIBLE "I'll solve a quadratic equation in the form y = ax^2 + bx + c."
VISIBLE "Enter the a coefficient::"
GIMMEH a
MAEK a A NUMBAR
VISIBLE "Enter the b coefficient::"
GIMMEH b
MAEK b A NUMBAR
VISIBLE "Enter the c coefficient::"
GIMMEH c
MAEK c A NUMBAR

BTW a = 0 -> linear
BOTH SAEM a AN 0, O RLY?
	YA RLY
		BOTH SAEM b AN 0, O RLY?
			YA RLY
				BTW b = 0 -> constant
				BOTH SAEM c AN 0, O RLY?
				YA RLY, VISIBLE "Constant function y = 0 -> any x is a solution"
				NO WAI, VISIBLE SMOOSH "Constant fucntion y = " c " -> no solution"
				OIC
			NO WAI
				BTW x = -c/b
				I HAS A x ITZ QUOSHUNT OF PRODUKT OF -1 AN c AN b
				VISIBLE SMOOSH "Linear function -> 1 solution x = " x MKAY
		OIC
		KTHX
OIC

BTW quadratic function
I HAS A delta
delta R DIFF OF PRODUKT OF b AN b AN PRODUKT OF 4 AN PRODUKT OF a AN c
VISIBLE SMOOSH "delta = " delta MKAY

BTW take all negative values down to -1 so that we can use a switch later on
BOTH SAEM -1 AN BIGGR OF delta AN -1
O RLY?
	YA RLY, delta R -1
OIC

delta, WTF?
	OMG -1
		VISIBLE "Negative delta -> equation has no solutions."
		GTFO
	OMG 0
		I HAS A x ITZ QUOSHUNT OF PRODUKT OF -1 AN b AN PRODUKT OF 2 AN a
		VISIBLE SMOOSH "Zero delta -> 1 solution x0 = " x MKAY
		GTFO
	OMGWTF
		BTW calculate the square root of delta
		I HAS A tmp ITZ delta
		I HAS A tmptwo ITZ 0
		I HAS A space ITZ delta
		I HAS A i ITZ 0
		IM IN YR LOOP UPPIN YR i TIL BOTH SAEM i AN 100
			tmptwo R PRODUKT OF tmp AN tmp
			BOTH SAEM delta AN tmptwo
			O RLY? YA RLY, GTFO, OIC

			space R PRODUKT OF 0.5 AN space

			BOTH SAEM delta AN SMALLR OF delta AN tmptwo
			O RLY?
				YA RLY, tmp R DIFF OF tmp AN space
				NO WAI, tmp R SUM OF tmp AN space
			OIC
		IM OUTTA YR LOOP
		delta R tmp

		BTW x1 = (-b - sqrt(delta))/2a
		I HAS A xone ITZ QUOSHUNT OF DIFF OF PRODUKT OF b AN -1 AN delta AN PRODUKT OF 2 AN a
		BTW x2 = (-b + sqrt(delta))/2a
		I HAS A xtwo ITZ QUOSHUNT OF SUM OF PRODUKT OF b AN -1 AN delta AN PRODUKT OF 2 AN a
		VISIBLE SMOOSH "Positive delta -> 2 solutions x1 = " xone " x2 = " xtwo MKAY
OIC

KTHXBYE