/*{
	"ISFVSN":"2.0",
	"TYPE":"IMAGE",
	"LABEL":"Solid Color",
	"INPUTS":[
		{
			"NAME":"fill",
			"TYPE":"color",
			"DEFAULT":[
				0.1,
				0.0,
				0.0,
				0.1
			]
		},
		{
			"NAME":"depth",
			"TYPE":"colorDepth"
		},
	],
}*/

void main()
{
	gl_FragColor = fill;
}