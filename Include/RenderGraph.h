#pragma once

#include <cstdint>
#include <vector>

namespace Renderer
{
	struct PipelineInfo
	{
		struct ShaderInfo
		{
			enum class Stage
			{
				VERTEX,
				FRAGMENT
			};

			const char * file_path;
			const char * entry_point;

			Stage stage;
		};

		unsigned short shader_count;

		ShaderInfo shaders[2];
	};

	struct AttachmentInfo
	{
		enum class Source
		{
			TRANSIENT,
			BACKBUFFER
		};

		const char * debug_name;

		Source source;
	};

	struct PassInfo
	{
		const char * debug_name;

		unsigned short color_output_count;
		unsigned short color_input_count;
		unsigned short pipeline_count;

		unsigned short color_outputs[8];
		unsigned short color_inputs[8];

		PipelineInfo * pipelines;
	};

	struct GraphInfo
	{
		unsigned short pass_count;
		unsigned short color_attachment_count;

		PassInfo * passes;

		AttachmentInfo * color_attachments;
	};
}
