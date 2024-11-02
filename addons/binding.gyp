{
	"targets": [
		{
			"target_name": "audio-io",
			"include_dirs": [
				"<!@(node -p \"require('node-addon-api').include\")",
				"include",
			],
			"defines": ["NAPI_CPP_EXCEPTIONS"],
			"cflags!": ["-fno-exceptions"],
			"cflags_cc!": ["-fno-exceptions"],
			"conditions": [
				[
					"OS=='win'",
					{
						"target_name": "windows-x64-mme-output",
						"libraries": ["winmm.lib"],
						"sources": ["src/windows-mme-output.cpp"],
						"msvs_settings": {"VCCLCompilerTool": {"ExceptionHandling": 1}},
					},
				],
				[
					"OS=='linux'",
					{

						"sources": ["src/linux-alsa-output.cpp"],
						"conditions": [
							[
								"target_arch=='x64'",
								{
									"target_name": "linux-x64-alsa-output",
									"libraries": ["-lasound"],
								},
							],
							[
								"target_arch=='arm64'",
								{
									"target_name": "linux-arm64-alsa-output",
									"cflags": [
										"-I~/arm64-libs/usr/include"
									],
									"ldflags": [
										"-L~/arm64-libs/usr/lib/aarch64-linux-gnu"
									],
								},
							],
						],
					},
				],
				[
					"OS=='mac'",
					{
						"sources": ["src/macos-coreaudio-output.cpp"],
						"xcode_settings": {
							"OTHER_LDFLAGS": ["-framework", "AudioUnit"],
							"GCC_ENABLE_CPP_EXCEPTIONS": "YES"
						},
						"conditions": [
							[
								"target_arch=='x64'",
								{
									"target_name": "macos-x64-coreaudio-output",
								},
							],
							[
								"target_arch=='arm64'",
								{
									"target_name": "macos-arm64-coreaudio-output",
								},
							],
						],
					},
				],
			],
		}
	]
}
