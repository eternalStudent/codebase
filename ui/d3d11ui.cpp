struct D3D11Quad {
	Point2 pos0;
	Point2 pos1;
	Color color01;
	Color color00;
	Color color11;
	Color color10;
	float32 cornerRadius;
	float32 borderThickness;
	Color borderColor;
};

struct D3D11Glyph {
	Point2 pos0;
	Point2 pos1;
	Point2 uv0;
	Point2 uv1;
	Color color;
};

struct D3D11Rectangle {
	Point2 pos0;
	Point2 pos1;
};

struct D3D11Segment {
	Point2 pos0;
	Point2 pos1;
	Point2 pos2;
	Point2 pos3;
	Color color;
};

struct D3D11Shadow {
	Point2 pos0;
	Point2 pos1;
	float32 cornerRadius;
	float32 blur;
	Color color;
};

struct D3D11Program {
	UINT stride;
	ID3D11VertexShader* vshader;
	ID3D11PixelShader* pshader;
	ID3D11InputLayout* layout;

	D3D11Texture texture;
	Dimensions2 textureDim;
};

enum {
	D3D11_BILINEAR,
	D3D11_NEAREST,

	D3D11_FILTER_COUNT
};

struct {
	ID3D11Device1* device;
	ID3D11DeviceContext1* context;
	IDXGISwapChain1* swapChain;
	ID3D11Buffer* vbuffer;
	ID3D11Buffer* mvpBuffer;
	ID3D11Buffer* cbuffer;
	ID3D11RenderTargetView* rtView;
	ID3D11RasterizerState1* ss_rasterizer;
	ID3D11RasterizerState1* ms_rasterizer;
	ID3D11BlendState* blend;

	union {
		ID3D11SamplerState* samplers[D3D11_FILTER_COUNT];
		struct {
			ID3D11SamplerState* bilinearSampler;
			ID3D11SamplerState* nearestSampler;
		};
	};

	D3D11Program quadProgram;
	D3D11Program glyphProgram;
	D3D11Program segmentProgram;
	D3D11Program imageProgram;
	D3D11Program shadowProgram;
	D3D11Program* currentProgram;
	
	int32 quadCount;
	Dimensions2i dim;
	FLOAT background[4];
	D3D11_MAPPED_SUBRESOURCE mapped;
} d3d11;

static char quadCode[] = 
"	#line " STRINGIFY(__LINE__) "\n"
R"STRING(
	struct VS_INPUT	{
		float2 pos0 : POS0;
		float2 pos1 : POS1;
		float4 color01 : COLOR0;
		float4 color00 : COLOR1;
		float4 color11 : COLOR2;
		float4 color10 : COLOR3;
		float radius : RAD;
		float bThickness : BTHICK;
		float4 bColor : BCOL;	
		uint vertexId : SV_VertexID;
	};				
					
	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float4 color : COLOR;
		float2 center : POS;
		float2 size : SIZE;
		float radius : RAD;
		float bThickness : BTHICK;
		float4 bColor : BCOL;
	};				
					
	cbuffer cbuffer0 : register(b0) {
		row_major float4x4 mvp;
	}				
					
	PS_INPUT vs(VS_INPUT input) {
		PS_INPUT output;
		float2 pixel_poses[] = {
			float2(input.pos0.x, input.pos1.y),
			float2(input.pos0.x, input.pos0.y),
			float2(input.pos1.x, input.pos1.y),
			float2(input.pos1.x, input.pos0.y),
		};			
		float2 pixel_pos = pixel_poses[input.vertexId];
		float4 pos = mul(mvp, float4(pixel_pos, 0, 1));
		output.pos = pos;
		float4 color[] = {
			input.color01,
			input.color00,
			input.color11,
			input.color10,
		};			
		output.size = float2(	
			(input.pos1.x - input.pos0.x)/2,
			(input.pos1.y - input.pos0.y)/2 
		);			
		output.center = input.pos0 + output.size;
		output.color = color[input.vertexId];
		output.radius = input.radius;
		output.bColor = input.bColor;
		output.bThickness = input.bThickness;
		return output;
	}				
					
	float sd(float2 pos, float2 halfSize, float radius) {
		pos = abs(pos) - halfSize + radius;	
		return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
	}				
					
	float4 ps(PS_INPUT input) : SV_TARGET	
	{				
		float2 pos = input.pos.xy;
		float4 color = input.color;
		float sd_ = sd(pos - input.center, input.size, input.radius);
		float sd2 = sd_ + input.bThickness;	
		float a = 1 - smoothstep(-0.5f, +0.5f, sd_);
		float a2 = 1 - smoothstep(-0.5f, +0.5f, sd2);
		color = lerp(input.bColor, input.color, a2);
		color.a *= a;
		return color;
	}				
)STRING";

static char glyphCode[] = 
"	#line " STRINGIFY(__LINE__) "\n"
R"STRING(
	struct VS_INPUT	{
		float2 pos0 : POS0;
		float2 pos1 : POS1;
		float2 uv0 : UV0;
		float2 uv1 : UV1;
		float4 color : COLOR;	
		uint vertexId : SV_VertexID;
	};				
					
	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float2 uv : UV;
		float4 color : COLOR;
	};				
					
	cbuffer cbuffer0 : register(b0) {
		row_major float4x4 mvp;
	}			

	cbuffer cbuffer1 : register(b1) {
		float2 atlasDim;
	}

	Texture2D<float> atlas : register(t0);
	SamplerState lsampler : register(s0);
					
	PS_INPUT vs(VS_INPUT input) {
		PS_INPUT output;
		float2 pixel_poses[] = {
			float2(input.pos0.x, input.pos1.y),
			float2(input.pos0.x, input.pos0.y),
			float2(input.pos1.x, input.pos1.y),
			float2(input.pos1.x, input.pos0.y),
		};
		float2 pixel_pos = pixel_poses[input.vertexId];
		output.pos = mul(mvp, float4(pixel_pos, 0, 1));

		float2 uvs[] = {
			float2(input.uv0.x, input.uv1.y),
			float2(input.uv0.x, input.uv0.y),
			float2(input.uv1.x, input.uv1.y),
			float2(input.uv1.x, input.uv0.y),
		};
		output.uv = uvs[input.vertexId] / atlasDim;

		output.color = input.color;

		return output;
	}			
					
	float4 ps(PS_INPUT input) : SV_TARGET {				
		float a = atlas.Sample(lsampler, input.uv);
		if (a == 0) discard;
		float4 color = input.color;
		color.a *= a;
 		return color;
	}				
)STRING";

static char imageCode[] = 
"	#line " STRINGIFY(__LINE__) "\n"
R"STRING(
	struct VS_INPUT	{
		float2 pos0 : POS0;
		float2 pos1 : POS1;
		uint vertexId : SV_VertexID;
	};				
					
	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float2 uv : UV;
	};				
					
	cbuffer cbuffer0 : register(b0) {
		row_major float4x4 mvp;
	}

	Texture2D<float4> image : register(t0);
	SamplerState lsampler : register(s0);
					
	PS_INPUT vs(VS_INPUT input) {
		PS_INPUT output;
		float2 pixel_poses[] = {
			float2(input.pos0.x, input.pos1.y),
			float2(input.pos0.x, input.pos0.y),
			float2(input.pos1.x, input.pos1.y),
			float2(input.pos1.x, input.pos0.y),
		};
		float2 pixel_pos = pixel_poses[input.vertexId];
		output.pos = mul(mvp, float4(pixel_pos, 0, 1));

		float2 uvs[] = {
			float2(0, 0),
			float2(0, 1),
			float2(1, 0),
			float2(1, 1),
		};
		output.uv = uvs[input.vertexId];

		return output;
	}			
					
	float4 ps(PS_INPUT input) : SV_TARGET {				
		float4 color = image.Sample(lsampler, input.uv);
 		return color;
	}				
)STRING";

char segmentCode[] = 
"	#line " STRINGIFY(__LINE__) "\n"
R"STRING(
	struct VS_INPUT	{
		float2 pos0 : POS0;
		float2 pos1 : POS1;
		float2 pos2 : POS2;
		float2 pos3 : POS3;
		float4 color : COLOR;
		uint vertexId : SV_VertexID;
	};

	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float4 color : COLOR;
	};	

	cbuffer cbuffer0 : register(b0) {
		row_major float4x4 mvp;
	}

	PS_INPUT vs(VS_INPUT input) {
		PS_INPUT output;

		float2 poses[4] = {
			input.pos0,
			input.pos1,
			input.pos2,
			input.pos3,
		};
		float2 pixelpos = poses[input.vertexId];
		output.pos = mul(mvp, float4(pixelpos, 0.0, 1.0));
		output.color = input.color;
		return output;
	}

	float4 ps(PS_INPUT input) : SV_TARGET {				
 		return input.color;
	}
)STRING";

static char shadowCode[] = 
"	#line " STRINGIFY(__LINE__) "\n"
R"STRING(
	struct VS_INPUT	{
		float2 pos0 : POS0;
		float2 pos1 : POS1;
		float radius : RAD;
		float blur : BLUR;
		float4 color : COLOR;
		uint vertexId : SV_VertexID;
	};				
					
	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float2 center : POS;
		float2 size : SIZE;
		float radius : RAD;
		float blur : BLUR;
		float4 color : COLOR;
	};				
					
	cbuffer cbuffer0 : register(b0) {
		row_major float4x4 mvp;
	}				
					
	PS_INPUT vs(VS_INPUT input) {
		PS_INPUT output;
		float2 pixel_poses[] = {
			float2(input.pos0.x, input.pos1.y),
			float2(input.pos0.x, input.pos0.y),
			float2(input.pos1.x, input.pos1.y),
			float2(input.pos1.x, input.pos0.y),
		};			
		float2 pixel_pos = pixel_poses[input.vertexId];
		float4 pos = mul(mvp, float4(pixel_pos, 0, 1));
		output.pos = pos;
		
		output.size = float2(	
			(input.pos1.x - input.pos0.x)/2,
			(input.pos1.y - input.pos0.y)/2 
		);			
		output.center = input.pos0 + output.size;
		output.radius = input.radius;
		output.blur = input.blur;
		output.color = input.color;
		return output;
	}				
					
	float sd(float2 pos, float2 halfSize, float radius) {
		pos = abs(pos) - halfSize + radius;	
		return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
	}				
					
	float4 ps(PS_INPUT input) : SV_TARGET	
	{				
		float2 pos = input.pos.xy;
		float sd_ = sd(pos - input.center, input.size, input.radius + input.blur);
		float a = 1 - smoothstep(-input.blur, 0.5f, sd_);
		float4 color = input.color;
		color.a *= a;
		return color;
	}				
)STRING";

// TODO: move some of this to d3d11.cpp
D3D11Program CreateProgram(String hlsl, D3D11_INPUT_ELEMENT_DESC* desc, UINT numElements, UINT stride) {
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef DEBUG
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	//flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_SKIP_VALIDATION;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* error;

	ID3DBlob* vblob;
	HRESULT hr = D3DCompile(hlsl.data, hlsl.length, NULL, NULL, NULL, "vs", "vs_5_0", flags, 0, &vblob, &error);
	if (FAILED(hr)){
		const char* message = (const char*)error->GetBufferPointer();
		OutputDebugStringA(message);
		ASSERT(!"Failed to compile vertex shader!");
	}

	ID3DBlob* pblob;
	hr = D3DCompile(hlsl.data, hlsl.length, NULL, NULL, NULL, "ps", "ps_5_0", flags, 0, &pblob, &error);
	if (FAILED(hr)) {
		const char* message = (const char*)error->GetBufferPointer();
		OutputDebugStringA(message);
		ASSERT(!"Failed to compile pixel shader!");
	}

	D3D11Program program = {stride};
	d3d11.device->CreateVertexShader(vblob->GetBufferPointer(), vblob->GetBufferSize(), NULL, &program.vshader);
	d3d11.device->CreatePixelShader(pblob->GetBufferPointer(), pblob->GetBufferSize(), NULL, &program.pshader);
	d3d11.device->CreateInputLayout(desc, numElements, vblob->GetBufferPointer(), vblob->GetBufferSize(), &program.layout);

	pblob->Release();
	vblob->Release();

	return program;
}

D3D11Texture D3D11GenerateTexture(Image image, int32 filter) {
	ID3D11ShaderResourceView* resource = GenerateTexture(d3d11.device, image);
	return {resource, d3d11.samplers[filter]};
}

void D3D11UIInit(uint32 globalFlags) {
	HRESULT hr;

	d3d11 = {};
	// create D3D11 device & context
	{
		ID3D11Device* base_device;
		ID3D11DeviceContext* base_context;

		UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG
		// this enables VERY USEFUL debug messages in debugger output
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
		hr = D3D11CreateDevice(
			NULL, 
			D3D_DRIVER_TYPE_HARDWARE, 
			NULL, 
			flags, 
			levels, 
			ARRAYSIZE(levels),
			D3D11_SDK_VERSION, 
			&base_device, 
			NULL, 
			&base_context);
		ASSERT_HR(hr);

		hr = base_device->QueryInterface(__uuidof(ID3D11Device1), (void **)(&d3d11.device));
		ASSERT_HR(hr);
		hr = base_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)(&d3d11.context));
		ASSERT_HR(hr);

		base_device->Release();
		base_context->Release();
	}

#ifdef DEBUG
	// for debug builds enable VERY USEFUL debug break on API errors
	{
		ID3D11InfoQueue* info;
		d3d11.device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		info->Release();
	}

	// enable debug break for DXGI too
	{
		IDXGIInfoQueue* dxgiInfo;
		hr = DXGIGetDebugInterface1(0, IID_IDXGIInfoQueue, (void**)&dxgiInfo);
		ASSERT_HR(hr);
		dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
		dxgiInfo->Release();
	}

	// after this there's no need to check for any errors on device functions manually
	// so all HRESULT return  values in this code will be ignored
	// debugger will break on errors anyway
#endif

	{
		// get DXGI device from D3D11 device
		IDXGIDevice1* dxgiDevice;
		hr = d3d11.device->QueryInterface(IID_IDXGIDevice1, (void**)&dxgiDevice);
		ASSERT_HR(hr);

		// get DXGI adapter from DXGI device
		IDXGIAdapter* dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		ASSERT_HR(hr);

		// get DXGI factory from DXGI adapter
		IDXGIFactory2* factory;
		hr = dxgiAdapter->GetParent(IID_IDXGIFactory2, (void**)&factory);
		ASSERT_HR(hr);

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		{
			// default 0 value for width & height means to get it from HWND automatically
			desc.Width              = 0;
			desc.Height             = 0;

			desc.Format             = (globalFlags & GFX_SRGB) ? 
										DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : 
										DXGI_FORMAT_R8G8B8A8_UNORM;

			desc.Stereo             = FALSE;

			desc.SampleDesc.Count   = (globalFlags & GFX_MULTISAMPLE) ? 4 : 1;
			desc.SampleDesc.Quality = 0;

			desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount        = 2;

			desc.Scaling            = DXGI_SCALING_STRETCH;

			desc.SwapEffect         = ((globalFlags & GFX_MULTISAMPLE) || (globalFlags & GFX_SRGB)) ? 
										DXGI_SWAP_EFFECT_DISCARD : 
										DXGI_SWAP_EFFECT_FLIP_DISCARD;

			desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
			desc.Flags              = 0;
		}

		hr = factory->CreateSwapChainForHwnd((IUnknown*)d3d11.device, window.handle, &desc, NULL, NULL, &d3d11.swapChain);
		ASSERT_HR(hr);

		// disable silly Alt+Enter changing monitor resolution to match window size
		factory->MakeWindowAssociation(window.handle, DXGI_MWA_NO_ALT_ENTER);

		factory->Release();
		dxgiAdapter->Release();
		dxgiDevice->Release();
	}

	{
		D3D11_BLEND_DESC desc = {};
		{
			desc.RenderTarget[0].BlendEnable            = 1;
			desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA; 
			desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		hr = d3d11.device->CreateBlendState(&desc, &d3d11.blend);
		ASSERT_HR(hr);
	}

	{
		D3D11_RASTERIZER_DESC1 desc = {};
		{
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.ScissorEnable = (globalFlags & GFX_SCISSOR) ? TRUE : FALSE;
			desc.MultisampleEnable = FALSE;
		}
		hr = d3d11.device->CreateRasterizerState1(&desc, &d3d11.ss_rasterizer);
		ASSERT_HR(hr);
	}

	{
		D3D11_RASTERIZER_DESC1 desc = {};
		{
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.ScissorEnable = (globalFlags & GFX_SCISSOR) ? TRUE : FALSE;;
			desc.MultisampleEnable = TRUE;
		}
		hr = d3d11.device->CreateRasterizerState1(&desc, &d3d11.ms_rasterizer);
		ASSERT_HR(hr);
	}

	{
		D3D11_BUFFER_DESC desc = {};
		{
			desc.ByteWidth = 1024 * 1024;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		};

		hr = d3d11.device->CreateBuffer(&desc, NULL, &d3d11.vbuffer);
		ASSERT_HR(hr);
	}

	{
		D3D11_BUFFER_DESC desc = {};
		{
			desc.ByteWidth = 4 * 4 * sizeof(float32);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		hr = d3d11.device->CreateBuffer(&desc, NULL, &d3d11.mvpBuffer);
		ASSERT_HR(hr);
	}

	{
		D3D11_BUFFER_DESC desc = {};
		{
			desc.ByteWidth = 16;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		hr = d3d11.device->CreateBuffer(&desc, NULL, &d3d11.cbuffer);
		ASSERT_HR(hr);
	}

	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		d3d11.device->CreateSamplerState(&desc, &d3d11.bilinearSampler);
	}

	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		d3d11.device->CreateSamplerState(&desc, &d3d11.nearestSampler);
	}

	{
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "POS"   , 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(struct D3D11Quad, pos0), 			D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 1, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(struct D3D11Quad, pos1), 			D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Quad, color01),   		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Quad, color00),   		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Quad, color11),   		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Quad, color10),   		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "RAD"   , 0, DXGI_FORMAT_R32_FLOAT,          0, offsetof(struct D3D11Quad, cornerRadius), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "BTHICK", 0, DXGI_FORMAT_R32_FLOAT,          0, offsetof(struct D3D11Quad, borderThickness), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "BCOL"  , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Quad, borderColor), 		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		d3d11.quadProgram = CreateProgram(STR(quadCode), desc, ARRAYSIZE(desc), sizeof(D3D11Quad));
	}

	{
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "POS"   , 0, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Glyph, pos0), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 1, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Glyph, pos1), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "UV"    , 0, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Glyph, uv0),	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "UV"    , 1, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Glyph, uv1),	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(struct D3D11Glyph, color),	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		d3d11.glyphProgram = CreateProgram(STR(glyphCode), desc, ARRAYSIZE(desc), sizeof(D3D11Glyph));
	}

	{
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "POS"   , 0, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Rectangle, pos0), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 1, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Rectangle, pos1), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		d3d11.imageProgram = CreateProgram(STR(imageCode), desc, ARRAYSIZE(desc), sizeof(D3D11Rectangle));
	}

	{
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "POS"   , 0, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Segment, pos0), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 1, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Segment, pos1), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 2, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Segment, pos2), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS"   , 3, DXGI_FORMAT_R32G32_FLOAT, 		0, offsetof(struct D3D11Segment, pos3),		D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 	0, offsetof(struct D3D11Segment, color), 	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		d3d11.segmentProgram = CreateProgram(STR(segmentCode), desc, ARRAYSIZE(desc), sizeof(D3D11Segment));
	}

	{
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "POS" ,  0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(struct D3D11Shadow, pos0), 		 D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "POS" ,  1, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(struct D3D11Shadow, pos1), 		 D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "RAD" ,  0, DXGI_FORMAT_R32_FLOAT,          0, offsetof(struct D3D11Shadow, cornerRadius), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "BLUR",  0, DXGI_FORMAT_R32_FLOAT,          0, offsetof(struct D3D11Shadow, blur),    	 D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct D3D11Shadow, color),	     D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		d3d11.shadowProgram = CreateProgram(STR(shadowCode), desc, ARRAYSIZE(desc), sizeof(D3D11Shadow));
	}
}

void D3D11UIBeginDrawing() {
	Dimensions2i dim = OSGetWindowDimensions();
	D3D11_RECT rect = {};
	rect.left = 0;
	rect.right = dim.width;
	rect.top = 0;
	rect.bottom = dim.height;
	d3d11.context->RSSetScissorRects(1, &rect);

	if (d3d11.rtView == NULL || d3d11.dim.width != dim.width || d3d11.dim.height != dim.height) {

		if (d3d11.rtView) {
			// release old swap chain buffers
			d3d11.context->ClearState();
			d3d11.rtView->Release();
			d3d11.rtView = NULL;
		}

		// resize to new size for non-zero size
		if (dim.width != 0 || dim.height != 0) {
			d3d11.swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			// create RenderTarget view for new backbuffer texture
			ID3D11Texture2D* backbuffer;
			d3d11.swapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
			d3d11.device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &d3d11.rtView);

			backbuffer->Release();
		}
	}

	d3d11.dim = dim;
	d3d11.quadCount = 0;

	// minimized
	if (d3d11.rtView == NULL) return;

	D3D11_VIEWPORT viewport = {};
	{
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (FLOAT)d3d11.dim.width;
		viewport.Height = (FLOAT)d3d11.dim.height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
	}
	d3d11.context->RSSetViewports(1, &viewport);
	d3d11.context->OMSetRenderTargets(1, &d3d11.rtView, NULL);
	d3d11.context->OMSetBlendState(d3d11.blend, 0, 0xffffffff);
	
	// clear screen
	d3d11.context->ClearRenderTargetView(d3d11.rtView, d3d11.background);
}

void EnableMultiSample() {
	d3d11.context->RSSetState(d3d11.ms_rasterizer);
}

void DisableMultiSample() {
	d3d11.context->RSSetState(d3d11.ss_rasterizer);
}

void PixelSpaceYIsDown() {
	float32 w = (float32)d3d11.dim.width;
	float32 h = (float32)d3d11.dim.height;
	
	float32 matrix[16] =
		{
			2.f/w, 0,      0, -1,
			0,     -2.f/h, 0,  1,
			0,     0,      0,  0,
			0,     0,      0,  1,
		};

	D3D11_MAPPED_SUBRESOURCE mapped;
	d3d11.context->Map((ID3D11Resource*)d3d11.mvpBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &matrix, sizeof(matrix));
	d3d11.context->Unmap((ID3D11Resource*)d3d11.mvpBuffer, 0);
	d3d11.context->VSSetConstantBuffers(0, 1, &d3d11.mvpBuffer);
}

void D3D11UISetBackgroundColor(Color color) {
	memcpy(&d3d11.background, &color, 4*sizeof(FLOAT));
}

void D3D11UISetFontAtlas(Image image) {
	// NOTE: Nearest works if this is a bit font, and also should work for regular font assuming no resizing
	d3d11.glyphProgram.texture = D3D11GenerateTexture(image, D3D11_NEAREST);
	d3d11.glyphProgram.textureDim = {(float32)image.dimensions.width, (float32)image.dimensions.height};
}

void FlushVertices() {
	if (!d3d11.quadCount) return;

	d3d11.context->Unmap((ID3D11Resource*)d3d11.vbuffer, 0);

	// Input Assembler
	d3d11.context->IASetInputLayout(d3d11.currentProgram->layout);
	d3d11.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	UINT stride = d3d11.currentProgram->stride;
	UINT offset = 0;
	d3d11.context->IASetVertexBuffers(0, 1, &d3d11.vbuffer, &stride, &offset);

	d3d11.context->VSSetShader(d3d11.currentProgram->vshader, NULL, 0);
	d3d11.context->PSSetShader(d3d11.currentProgram->pshader, NULL, 0);

	PixelSpaceYIsDown();
	if (d3d11.currentProgram->texture.resource)
		d3d11.context->PSSetShaderResources(0, 1, &d3d11.currentProgram->texture.resource);
	if (d3d11.currentProgram->texture.sampler)
		d3d11.context->PSSetSamplers(0, 1, &d3d11.currentProgram->texture.sampler);
	if (d3d11.currentProgram == &d3d11.glyphProgram) {
		// TODO: I don't love this
		D3D11_MAPPED_SUBRESOURCE mapped;
		d3d11.context->Map((ID3D11Resource*)d3d11.cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		memcpy(mapped.pData, &d3d11.glyphProgram.textureDim, sizeof(d3d11.glyphProgram.textureDim));
		d3d11.context->Unmap((ID3D11Resource*)d3d11.cbuffer, 0);
		d3d11.context->VSSetConstantBuffers(1, 1, &d3d11.cbuffer);
	}

	d3d11.context->DrawInstanced(4, d3d11.quadCount, 0, 0);
	d3d11.quadCount = 0;
}

void DrawQuad(D3D11Quad quad) {
	if (d3d11.currentProgram != &d3d11.quadProgram) {
		FlushVertices();
		d3d11.currentProgram = &d3d11.quadProgram;
		DisableMultiSample();
	}

	if (d3d11.quadCount == 0)
		d3d11.context->Map((ID3D11Resource*)d3d11.vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11.mapped);

	memcpy((byte*)d3d11.mapped.pData + d3d11.quadCount*sizeof(D3D11Quad), &quad, sizeof(D3D11Quad));
	d3d11.quadCount++;
}

void D3D11UIDrawImage(D3D11Texture image, Point2 pos, Point2 dim) {
	FlushVertices();
	DisableMultiSample();
	PixelSpaceYIsDown();

	d3d11.context->Map((ID3D11Resource*)d3d11.vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11.mapped);
	D3D11Rectangle rect = {pos, pos+dim};
	memcpy((byte*)d3d11.mapped.pData, &rect, sizeof(D3D11Rectangle));
	d3d11.context->Unmap((ID3D11Resource*)d3d11.vbuffer, 0);

	// Input Assembler
	d3d11.context->IASetInputLayout(d3d11.imageProgram.layout);
	d3d11.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	UINT stride = d3d11.imageProgram.stride;
	UINT offset = 0;
	d3d11.context->IASetVertexBuffers(0, 1, &d3d11.vbuffer, &stride, &offset);

	d3d11.context->VSSetShader(d3d11.imageProgram.vshader, NULL, 0);
	d3d11.context->PSSetShader(d3d11.imageProgram.pshader, NULL, 0);
	d3d11.context->PSSetShaderResources(0, 1, &image.resource);
	d3d11.context->PSSetSamplers(0, 1, &image.sampler);

	d3d11.context->DrawInstanced(4, 1, 0, 0);
}

void OSD3D11SwapBuffers() {
	HRESULT hr = d3d11.swapChain->Present(1, 0);
	if (hr == DXGI_STATUS_OCCLUDED) {
		Sleep(10);
	}
	else {
		ASSERT_HR(hr);
	}
}

void D3D11UIEndDrawing() {
	FlushVertices();

	OSD3D11SwapBuffers();
}

void D3D11UIDrawSolidColorQuad(Point2 pos, Dimensions2 dim, Color color) {

	D3D11Quad quad = {
		pos,
		pos + dim,
		color,
		color,
		color, 
		color,
		0,
		0,
		{},
	};

	DrawQuad(quad);
}

void D3D11UIDrawSolidColorQuad(Box2 box, Color color) {

	D3D11Quad quad = {
		box.p0,
		box.p1,
		color,
		color,
		color, 
		color,
		0,
		0,
		{},
	};

	DrawQuad(quad);
}

void D3D11UIDrawSolidColorQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color, 
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		pos,
		pos + dim,
		color,
		color,
		color, 
		color,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawSolidColorQuad(
	Box2 box, 
	Color color,
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		box.p0,
		box.p1,
		color,
		color,
		color, 
		color,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawVerticalGradQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color1, 
	Color color2,
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		pos,
		pos + dim,
		color2,
		color1,
		color2, 
		color1,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawVerticalGradQuad(
	Box2 box, 
	Color color1, 
	Color color2,
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		box.p0,
		box.p1,
		color2,
		color1,
		color2, 
		color1,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawHorizontalGradQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color1, 
	Color color2,
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		pos,
		pos + dim,
		color1,
		color1,
		color2, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawHorizontalGradQuad(
	Box2 box, 
	Color color1, 
	Color color2,
	float32 cornerRadius, 
	float32 borderThickness, 
	Color borderColor) {

	D3D11Quad quad = {
		box.p0,
		box.p1,
		color1,
		color1,
		color2, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawOutlineQuad(Point2 pos, Dimensions2 dim, float32 thickness, Color color) {

	D3D11Quad quad = {
		pos,
		pos + dim,
		{},
		{},
		{}, 
		{},
		0,
		thickness,
		color,
	};

	DrawQuad(quad);
}

void D3D11UIDrawOutlineQuad(Box2 box, float32 thickness, Color color) {

	D3D11Quad quad = {
		box.p0,
		box.p1,
		{},
		{},
		{}, 
		{},
		0,
		thickness,
		color,
	};

	DrawQuad(quad);
}

void D3D11UIDrawSphere(Point2 center, float32 radius, Color color, float32 borderThickness, Color borderColor) {

	Point2 pos0 = {center.x - radius, center.y - radius};
	Point2 pos1 = {center.x + radius, center.y + radius};	

	D3D11Quad quad = {
		pos0,
		pos1,
		color,
		color,
		color, 
		color,
		radius,
		borderThickness,
		borderColor,
	};

	DrawQuad(quad);
}

void D3D11UIDrawGlyph(Point2 pos, Dimensions2 dim, Box2 crop, Color color) {
	if (d3d11.currentProgram != &d3d11.glyphProgram) {
		FlushVertices();
		d3d11.currentProgram = &d3d11.glyphProgram;
		DisableMultiSample();
	}

	D3D11Glyph glyph = {
		pos,
		pos + dim,
		crop.p0,
		crop.p1,
		color
	};

	if (d3d11.quadCount == 0)
		d3d11.context->Map((ID3D11Resource*)d3d11.vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11.mapped);

	memcpy((byte*)d3d11.mapped.pData + d3d11.quadCount*sizeof(D3D11Glyph), &glyph, sizeof(D3D11Glyph));
	d3d11.quadCount++;
}

void D3D11UIDrawCurve(Point2 p0, Point2 p1, Point2 p2, Point2 p3, float32 thick, Color color) {
	if (d3d11.currentProgram != &d3d11.segmentProgram) {
		FlushVertices();
		d3d11.currentProgram = &d3d11.segmentProgram;
		EnableMultiSample();
	}
	
#define SEGMENTS	32
	Point2 prev = p0;
    Point2 current = {};
	for (int32 i = 0; i < SEGMENTS; i++) {
	    float32 t = (i+1) / (float32)SEGMENTS;
	    current = Cerp(t, p0, p1, p2, p3);

		Point2 delta = {current.x - prev.x, current.y - prev.y};
		float32 length = sqrt(delta.x*delta.x + delta.y*delta.y);
		
		float32 size = (0.5f*thick)/length;
		Point2 radius = {-size*delta.y, size*delta.x};
		
		D3D11Segment segment = {
			prev + radius,
			prev - radius,
			current + radius,
			current - radius,
			color
		};

		if (d3d11.quadCount == 0)
			d3d11.context->Map((ID3D11Resource*)d3d11.vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11.mapped);

		memcpy((byte*)d3d11.mapped.pData + d3d11.quadCount*sizeof(D3D11Segment), &segment, sizeof(segment));
		d3d11.quadCount++;
		prev = current;
	}
#undef SEGMENTS
}

void D3D11UIDrawShadow(Point2 pos, Dimensions2 dim, float32 radius, Point2 offset, float32 blur, Color color) {
	if (d3d11.currentProgram != &d3d11.shadowProgram) {
		FlushVertices();
		d3d11.currentProgram = &d3d11.shadowProgram;
		DisableMultiSample();
	}

	D3D11Shadow shadow = {
		{pos.x + offset.x - blur/2,             pos.y + offset.y - blur/2}, 
		{pos.x + offset.x + blur/2 + dim.width, pos.y + offset.y + blur/2 + dim.height}, 
		radius, blur, color
	};

	if (d3d11.quadCount == 0)
		d3d11.context->Map((ID3D11Resource*)d3d11.vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11.mapped);

	memcpy((byte*)d3d11.mapped.pData + d3d11.quadCount*sizeof(D3D11Shadow), &shadow, sizeof(D3D11Shadow));
	d3d11.quadCount++;
}

void D3D11UICropScreen(LONG left, LONG top, LONG width, LONG height) {
	FlushVertices();

	D3D11_RECT rect = {};
	rect.left = left;
	rect.right = left + width;
	rect.top = top;
	rect.bottom = top + height;
	d3d11.context->RSSetScissorRects(1, &rect);
}

void D3D11UIClearCrop() {
	D3D11UICropScreen(0, 0, d3d11.dim.width, d3d11.dim.height); 
}