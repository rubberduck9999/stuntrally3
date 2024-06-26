@insertpiece( SetCrossPlatformSettings )

// START UNIFORM DECLARATION  PS  Unlit  hlsl
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION  PS  Unlit  hlsl

struct PS_INPUT
{
	@insertpiece( VStoPS_block )
};

@property( !hlms_shadowcaster || alpha_test )
	@foreach( num_textures, n )
		@property( is_texture@n_array )
			Texture2DArray textureMapsArray@n : register(t@value(textureMapsArray@n));
		@else
			Texture2D textureMaps@n : register(t@value(textureMaps@n));
		@end
	@end
@end

@foreach( num_samplers, n )
	SamplerState samplerState@n : register(s@counter(samplerStateStart));@end

@insertpiece( DeclOutputType )

@insertpiece( DefaultHeaderPS )

@property( hlms_shadowcaster )
	@insertpiece( DeclShadowCasterMacros )
@end

@insertpiece( output_type ) main
(
	PS_INPUT inPs
	@property( hlms_vpos ), float4 gl_FragCoord : SV_Position@end
)
{
	PS_OUTPUT outPs;
	@insertpiece( custom_ps_preExecution )
	@property( !hlms_shadowcaster || alpha_test )
		@insertpiece( DefaultBodyPS )
	@end
	@property( hlms_shadowcaster )
		@insertpiece( DoShadowCastPS )
	@end
	@insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only )
	return outPs;
@end
}
