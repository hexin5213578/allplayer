Texture2D<float> luminanceChannel : t0;
Texture2D<float2> chrominanceChannel : t1;

SamplerState splr;

struct MosaicSetting
{
    int MosaicPixelSize;
};
struct BSmoothSetting
{
    float2 pixelSize;
    float iteration;
    int whiteLine;
};
struct SobelEdge
{
    int mode;
};
struct SharpenSetting
{
    float offset;
};

cbuffer Cbuf
{
    int whatEnable;
    MosaicSetting mosaicSetting;
    BSmoothSetting bSmoothSetting;
    SobelEdge sobelEdgeSetting;
    SharpenSetting sharpenSetting;
};

static const float3x3 YUVtoRGBCoeffMatrix =
{
    1.164383f, 1.164383f, 1.164383f,
	0.000000f, -0.391762f, 2.017232f,
	1.596027f, -0.812968f, 0.000000f
};

float3 ConvertYUVtoRGB(float3 yuv)
{
	// Derived from https://msdn.microsoft.com/en-us/library/windows/desktop/dd206750(v=vs.85).aspx
	// Section: Converting 8-bit YUV to RGB888

	// These values are calculated from (16 / 255) and (128 / 255)
    yuv -= float3(0.062745f, 0.501960f, 0.501960f);
    yuv = mul(yuv, YUVtoRGBCoeffMatrix);

    return saturate(yuv);
}

float3 GetYUVColor(float2 tc)
{
    float y = luminanceChannel.Sample(splr, tc);
    float2 uv = chrominanceChannel.Sample(splr, tc);
    return float3(y, uv);
}

float3 GetRGBFromTexture(float2 tc)
{
    return ConvertYUVtoRGB(GetYUVColor(tc));
}

float4 GetRGBAFromTexture(float2 tc)
{
    return float4(GetRGBFromTexture(tc), 1);
}

float3 Mosaic(float2 tc, int PixelSize)
{
    float3 col = float3(0, 0, 0);
    
    tc += 0.005;
    float2 ratio = floor(tc * PixelSize) / PixelSize;
    
    return ConvertYUVtoRGB(GetYUVColor(ratio));
}

float4 bSmooth(float2 tex)
{
    uint width;
    uint height;
    luminanceChannel.GetDimensions(width, height);
    float2 pp = float2(1.0 / width, 1.0 / height) * bSmoothSetting.pixelSize.x;
    
    static const float kernel[] =
    {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };
    
    float3 color = float3(0, 0, 0);
    uint index = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            color += kernel[index] * GetRGBFromTexture(tex + float2(i * pp.x, j * pp.y));
            index++;
        }
    }

    return float4(color / (16), 1);
}

float4 Sobel_Edge(float2 tex)
{
    const float4 CoefLuma = float4(0.212656, 0.715158, 0.072186, 0);
    const int Mode = sobelEdgeSetting.mode;
    const float T_Sobel = 0.15;
    const float WhiteLimit = 180.0 / 255.0;
    const float SatFactor = 1.8;
    
    const float2 p1 = float2(0.001, 0.001);
    float px = p1.x;
    float py = p1.y;
    
    float4 c8 = GetRGBAFromTexture(tex + p1);
    c8 -= GetRGBAFromTexture(tex - p1);
    float4 c3 = GetRGBAFromTexture(tex + float2(px, -py));
    c3 -= GetRGBAFromTexture(tex + float2(-px, py));
    float4 c4 = GetRGBAFromTexture(tex + float2(-px, 0));
    float4 c5 = GetRGBAFromTexture(tex + float2(px, 0));
    float2 g;
    g.x = dot(c8 + c3 - 2 * c4 + 2 * c5, CoefLuma);
    float4 c7 = GetRGBAFromTexture(tex + float2(0, py));
    float4 c2 = GetRGBAFromTexture(tex + float2(0, -py));
    g.y = dot(c8 - c3 + 2 * c7 - 2 * c2, CoefLuma);
    float edge = length(g);
	
    if (Mode == 0)	//Sobel
        return (edge > T_Sobel) ? min(edge, WhiteLimit) : 0;
    if (Mode == 1) //Inverted_Sobel: dark grey on white background 
        return (edge < T_Sobel) ? WhiteLimit : 1 - 1.4 * max(edge, 0.25);
    else
    {
        float4 color = GetRGBAFromTexture(tex);
        float gray = dot(color, CoefLuma); //1/3. 
        color = (edge < T_Sobel) ? color : color * saturate(1 - 1.1 * edge);
        return lerp(gray, color, SatFactor);
    }
}

float4 Sharpen(float2 tex)
{
    uint width;
    uint height;
    luminanceChannel.GetDimensions(width, height);
    float2 pp = float2(1.0 / width, 1.0 / height) * sharpenSetting.offset;
    
    static const int kernel[] =
    {
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0
    };
    
    float3 color = float3(0, 0, 0);
    uint index = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            int v = kernel[index];
            if (v != 0)
            {
                color += v * GetRGBFromTexture(tex + float2(i * pp.x, j * pp.y));
            }
            index++;
        }
    }

    return float4(color, 1);
}

float4 Test(float2 tex)
{
    uint width;
    uint height;
    luminanceChannel.GetDimensions(width, height);
    float2 pp = float2(1.0 / width, 1.0 / height) * mosaicSetting.MosaicPixelSize;
    
    static const float kernel[] = {
        1,2,1,
        2,4,2,
        1,2,1
    };
    
    float3 color = float3(0, 0, 0);
    uint index = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            color += kernel[index] * GetRGBFromTexture(tex + float2(i * pp.x, j * pp.y));
            index++;
        }
    }

    return float4(color / (16), 1);
}

float4 PS2(float2 tc : TexCoord) : SV_Target
{   
    if (whatEnable == 1)
    {
        return float4(Mosaic(tc, mosaicSetting.MosaicPixelSize), 1);
    }
    else if (whatEnable == 2)
    {
        return bSmooth(tc);
    }
    else if (whatEnable == 3)
    {
        return Sobel_Edge(tc);
    }
    else if (whatEnable == 4)
    {
        return Sharpen(tc);

    }
    else
    {
        return float4(GetRGBFromTexture(tc), 1);
    }
}