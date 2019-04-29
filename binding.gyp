{
	"targets": [
		{
			"target_name": "binding",
			"link_settings": {
				"library_dirs": [ "clang/lib/" ],
				"libraries": [ "libclang.lib" ],
			},
			"copies": [
				{
					"destination": "<(PRODUCT_DIR)",
					"files": [ "clang/bin/libclang.dll" ],
				}
			],
			"sources": [ "binding.cpp" ],
			"include_dirs": [ "<!@(node -p \"require('node-addon-api').include\")", "clang/include" ],

			"defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
			'msvs_settings': {
				'VCCLCompilerTool': {
					'AdditionalOptions': [ '/std:c++17' ],
				},
			},
		}
	]
}
