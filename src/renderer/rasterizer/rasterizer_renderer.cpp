#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <gif.h>

#include "rasterizer_renderer.h"
#include "utils/resource_utils.h"


void cg::renderer::rasterization_renderer::init()
{
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);
	depth_buffer = std::make_shared<cg::resource<float>>(settings->width, settings->height);
	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();

	rasterizer->set_viewport(settings->width, settings->height);
	rasterizer->set_render_target(render_target, depth_buffer);

	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	for (size_t i = 0; i < model->get_index_buffers().size(); ++i) {
		auto vertex_buffer_size = model->get_vertex_buffers()[i]->size_bytes();
		auto index_buffer_size = model->get_index_buffers()[i]->size_bytes();

		auto pure_vertex_buffer_size = model->get_index_buffers()[i]->count() * sizeof(cg::vertex);

		std::cout << "Vertex buffer size: " << vertex_buffer_size << std::endl;
		std::cout << "Index buffer size: " << index_buffer_size << std::endl;

		std::cout << "Pure vertex buffer size: " << pure_vertex_buffer_size << std::endl;
		std::cout << "Saving: " << pure_vertex_buffer_size - vertex_buffer_size - index_buffer_size << std::endl;
	}

	camera = std::make_shared<cg::world::camera>();
	camera->set_height(float(settings->height));
	camera->set_width(float(settings->width));
	camera->set_position(float3{
			settings->camera_position[0],
			settings->camera_position[1],
			settings->camera_position[2],
	});
	camera->set_phi(settings->camera_phi);
	camera->set_theta(settings->camera_theta);
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_z_near(settings->camera_z_near);
	camera->set_z_far(settings->camera_z_far);
}

void cg::renderer::rasterization_renderer::render()
{
	float4x4 matrix = mul(
			camera->get_projection_matrix(),
			camera->get_view_matrix(),
			model->get_world_matrix()
	);

	rasterizer->vertex_shader = [&](float4 vertex, cg::vertex vertex_data) {
		float4 transformed = mul(matrix, vertex);
		return std::make_pair(transformed, vertex_data);
	};

	rasterizer->pixel_shader = [](cg::vertex vertex_data, float z) {
		return cg::color::from_float3(vertex_data.ambient);
	};

	GifWriter gif;
	size_t width = render_target->get_stride();
	size_t height = render_target->count() / width;
	GifBegin(&gif, "result.gif", width, height, 10);

	size_t frames = 50;
	for (size_t i = 0; i < frames; ++i) {
		auto start = std::chrono::high_resolution_clock ::now();
		rasterizer->clear_render_target({0, 0, 0});
		auto end = std::chrono::high_resolution_clock ::now();
		auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "Clearing: " << double(time.count()) / 1000.0 << " ms" << std::endl;

		for (size_t shape_id = 0; shape_id < model->get_index_buffers().size(); ++shape_id) {
			rasterizer->set_vertex_buffer(model->get_vertex_buffers()[shape_id]);
			rasterizer->set_index_buffer(model->get_index_buffers()[shape_id]);
			rasterizer->draw(model->get_index_buffers()[shape_id]->count(), 0);
		}

		float angle = 2 * (float) M_PI / (float) frames;
		matrix = linalg::mul(
				matrix,
				linalg::rotation_matrix(linalg::rotation_quat(float3{0, 1, 0}, angle))
		);

		std::vector<uint8_t> rgba;
		rgba.reserve(width * height * 4);

		for (size_t j = 0; j < width * height; ++j) {
			const auto& color = render_target->get_data()[j];
			rgba.emplace_back(color.r);
			rgba.emplace_back(color.g);
			rgba.emplace_back(color.b);
			rgba.emplace_back(255);
		}

		GifWriteFrame(&gif, rgba.data(), width, height, 10);
	}

	GifEnd(&gif);
	cg::utils::save_resource(*render_target, settings->result_path);
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}