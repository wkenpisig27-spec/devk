// Technique 't0' will render the sample scene in one pass. 
texture tex0;        // First texture

//model
technique t0
{
    pass p0
    {
		PixelShader  = NULL;

		ZEnable			 = TRUE;
		ZWriteEnable	 = FALSE;

		VertexBlend      = Disable;

		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = TRUE;

		CullMode		 = None;
		Lighting		 = FALSE;
		
		MinFilter[0]	 = Linear;
		MagFilter[0]	 = Linear;

		AddressU[0]=WRAP;
        AddressV[0]=WRAP;
        
        AlphaTestEnable  = FALSE;
        
		AlphaBlendEnable = TRUE;
		//SrcBlend		 = SrcAlpha;
		//DestBlend		 = InvSrcAlpha;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}

technique t1
{
    pass p0
    {
		PixelShader  = NULL;

		ZEnable			 = TRUE;
		ZWriteEnable	 = TRUE;

		CullMode		 = None;
		Lighting		 = FALSE;
		SpecularEnable	 = TRUE;
		
		AddressU[0]=WRAP;
        AddressV[0]=WRAP;
        
        AlphaTestEnable  = FALSE;
      
		AlphaBlendEnable = FALSE;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}
//shade
technique t2
{
    pass p0
    {
		PixelShader  = NULL;
		ZEnable			 = TRUE;
		ZWriteEnable	 = FALSE;
		
		VertexBlend      = Disable;
		
		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = FALSE;
		
		CullMode		 = None;
		Lighting		 = FALSE;
			
		MinFilter[0]	 = Linear;
		MagFilter[0]	 = Linear;

		AddressU[0]=CLAMP;
        AddressV[0]=CLAMP;
        
        AlphaTestEnable  = FALSE;
		AlphaBlendEnable = TRUE;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}
//partice
technique t3
{
    pass p0
    {
		PixelShader  = NULL;
		ZEnable			 = TRUE;
		ZWriteEnable	 = FALSE;
		
		VertexBlend      = Disable;
		
		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = FALSE;
		
		CullMode		 = None;
		Lighting		 = FALSE;
			
		AddressU[0]=CLAMP;
        AddressV[0]=CLAMP;
        
        AlphaTestEnable  = FALSE;
                
		AlphaBlendEnable = TRUE;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = TFactor;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = TFactor;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}
//shade2
technique t4
{
    pass p0
    {
		PixelShader  = NULL;
		ZEnable			 = TRUE;
		ZWriteEnable	 = FALSE;
		
		VertexBlend      = Disable;
		
		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = FALSE;
		
		CullMode		 = None;
		Lighting		 = FALSE;
			
		MinFilter[0]	 = Linear;
		MagFilter[0]	 = Linear;

		AddressU[0]=WRAP;
        AddressV[0]=WRAP;
        
        AlphaTestEnable  = TRUE;
        AlphaRef		 = 0xff000000;
        AlphaFunc		 = NOTEQUAL;
        
		AlphaBlendEnable = TRUE;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}
//font state
technique t5
{
    pass p0
    {
		PixelShader  = NULL;
		ZEnable			 = FALSE;
		ZWriteEnable	 = FALSE;
		
		VertexBlend      = Disable;
		
		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = FALSE;
		
		Clipping		 = FALSE;
		
		SHADEMODE		 = FLAT;
		
		CullMode		 = CCW;
		Lighting		 = FALSE;
			
		MinFilter[0]	 = Point;
		MagFilter[0]	 = Point;

		AddressU[0]=CLAMP;
        AddressV[0]=CLAMP;
        
        AlphaTestEnable  = FALSE;
		AlphaBlendEnable = TRUE;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}
}
//font com
technique t6
{
    pass p0
    {
		PixelShader  = NULL;
		ZEnable			 = FALSE;
		ZWriteEnable	 = FALSE;
		
		VertexBlend      = Disable;
		
		StencilEnable	 = FALSE;
		DitherEnable	 = FALSE;
		FogEnable		 = FALSE;
		SpecularEnable	 = FALSE;
		
		SHADEMODE		 = Flat;
		
		CullMode		 = CCW;
		Lighting		 = FALSE;
			
		MinFilter[0]	 = Linear;
		MagFilter[0]	 = Linear;

		AddressU[0]=WRAP;
        AddressV[0]=WRAP;
        
        AlphaTestEnable  = FALSE;
		AlphaBlendEnable = TRUE;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
		
        //Texture[0] = <tex0>;
        //Texture[1] = NULL;
        
        ColorOp[0] = Modulate;
        ColorArg1[0] = Texture;
        ColorArg2[0] = Diffuse;
 
        AlphaOp[0] = Modulate;
        AlphaArg1[0] = Texture;
        AlphaArg2[0] = Diffuse;
		        		
        ColorOp[1] = Disable;
        AlphaOp[1] = Disable;
	}

}