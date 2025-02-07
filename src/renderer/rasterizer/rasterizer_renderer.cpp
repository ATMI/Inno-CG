#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::rasterization_renderer::init()
{
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);
	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();

	rasterizer->set_viewport(settings->width, settings->height);
	rasterizer->set_render_target(render_target);

	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	for (size_t i = 0; i < model->get_index_buffers().size(); ++i) {
		auto vertex_buffer_size = model->get_vertex_buffers()[i]->size_bytes();
		auto index_buffer_size = model->get_index_buffers()[i]->size_bytes();

		auto pure_vertex_buffer_size = model->get_index_buffers()[i]->count() * sizeof(cg::vertex);
		auto pure_index_buffer_size = model->get_index_buffers()[i]->count() * sizeof(cg::vertex);

		std::cout << "Vertex buffer size: " << vertex_buffer_size << std::endl;
		std::cout << "Index buffer size: " << index_buffer_size << std::endl;

		std::cout << "Pure vertex buffer size: " << pure_vertex_buffer_size << std::endl;
		std::cout << "Saving: " << pure_vertex_buffer_size - vertex_buffer_size - index_buffer_size << std::endl;
	}
	// TODO Lab: 1.03 Adjust `cg::renderer::rasterization_renderer` class to consume `cg::world::model`
	// TODO Lab: 1.04 Setup an instance of camera `cg::world::camera` class in `cg::renderer::rasterization_renderer`
	// TODO Lab: 1.06 Add depth buffer in `cg::renderer::rasterization_renderer`
}
void cg::renderer::rasterization_renderer::render()
{
	auto start = std::chrono::high_resolution_clock ::now();
	rasterizer->clear_render_target({0, 255, 255});
	auto end = std::chrono::high_resolution_clock ::now();
	auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	std::cout << double(time.count()) / 1000.0 << " ms" << std::endl;

	// TODO Lab: 1.04 Implement `vertex_shader` lambda for the instance of `cg::renderer::rasterizer`
	// TODO Lab: 1.05 Implement `pixel_shader` lambda for the instance of `cg::renderer::rasterizer`
	// TODO Lab: 1.03 Adjust `cg::renderer::rasterization_renderer` class to consume `cg::world::model`

	for (size_t shape_id = 0; shape_id < model->get_index_buffers().size(); ++shape_id) {
		rasterizer->set_vertex_buffer(model->get_vertex_buffers()[shape_id]);
		rasterizer->set_index_buffer(model->get_index_buffers()[shape_id]);
		rasterizer->draw(model->get_index_buffers()[shape_id]->count(), 0);
	}

	cg::utils::save_resource(*render_target, settings->result_path);
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}