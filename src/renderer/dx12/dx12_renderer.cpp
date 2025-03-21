#include "dx12_renderer.h"

#include "utils/com_error_handler.h"
#include "utils/window.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>


void cg::renderer::dx12_renderer::init()
{
	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

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

	view_port = CD3DX12_VIEWPORT(0.0f, 0.0f, float(settings->width), float(settings->height));
	scissor_rect = CD3DX12_RECT(0, 0, long(settings->width), long(settings->height));

	load_pipeline();
	load_assets();
}

void cg::renderer::dx12_renderer::destroy()
{
	wait_for_gpu();
	CloseHandle(fence_event);
}

void cg::renderer::dx12_renderer::update()
{
	// TODO Lab: 3.08 Implement `update` method of `dx12_renderer`
}

void cg::renderer::dx12_renderer::render()
{
	// TODO Lab: 3.06 Implement `render` method
}

ComPtr<IDXGIFactory4> cg::renderer::dx12_renderer::get_dxgi_factory()
{
	UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ComPtr<IDXGIFactory4> dxgi_factory;
	THROW_IF_FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
	return dxgi_factory;
}

void cg::renderer::dx12_renderer::initialize_device(ComPtr<IDXGIFactory4>& dxgi_factory)
{
	ComPtr<IDXGIAdapter1> hw_adapter;
	dxgi_factory->EnumAdapters1(0, &hw_adapter);

#ifdef _DEBUG
	DXGI_ADAPTER_DESC adapter_desc = {};
	hw_adapter->GetDesc(&adapter_desc);
	OutputDebugString(adapter_desc.Description);
	OutputDebugString(L"\n");
#endif

	THROW_IF_FAILED(D3D12CreateDevice(hw_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
}

void cg::renderer::dx12_renderer::create_direct_command_queue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue));
}

void cg::renderer::dx12_renderer::create_swap_chain(ComPtr<IDXGIFactory4>& dxgi_factory)
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.BufferCount = frame_number;
	desc.Height = settings->height;
	desc.Width = settings->width;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> temp_swap_chain;
	THROW_IF_FAILED(
			dxgi_factory->CreateSwapChainForHwnd(
					command_queue.Get(),
					cg::utils::window::get_hwnd(),
					&desc,
					nullptr,
					nullptr,
					&temp_swap_chain));

	dxgi_factory->MakeWindowAssociation(cg::utils::window::get_hwnd(), DXGI_MWA_NO_ALT_ENTER);
	temp_swap_chain.As(&swap_chain);
	frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void cg::renderer::dx12_renderer::create_render_target_views()
{
	rtv_heap.create_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frame_number);
	for (UINT i = 0; i < frame_number; ++i) {
		THROW_IF_FAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])));
		device->CreateRenderTargetView(render_targets[i].Get(), nullptr, rtv_heap.get_cpu_descriptor_handle(i));

		std::wstring name(L"Render target ");
		name += i;
		render_targets[i]->SetName(name.c_str());
	}
}

void cg::renderer::dx12_renderer::create_depth_buffer()
{
}

void cg::renderer::dx12_renderer::create_command_allocators()
{
	// TODO Lab: 3.06 Create command allocators and a command list
}

void cg::renderer::dx12_renderer::create_command_list()
{
	// TODO Lab: 3.06 Create command allocators and a command list
}


void cg::renderer::dx12_renderer::load_pipeline()
{
	ComPtr<IDXGIFactory4> dxgi_factory = get_dxgi_factory();
	initialize_device(dxgi_factory);
	create_direct_command_queue();
	create_swap_chain(dxgi_factory);
	create_render_target_views();
}

D3D12_STATIC_SAMPLER_DESC cg::renderer::dx12_renderer::get_sampler_descriptor()
{
	D3D12_STATIC_SAMPLER_DESC sampler_desc{};
	return sampler_desc;
}

void cg::renderer::dx12_renderer::create_root_signature(
	const D3D12_STATIC_SAMPLER_DESC* sampler_descriptors,
	UINT num_sampler_descriptors
)
{
	CD3DX12_ROOT_PARAMETER1 root_parameters[1];
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

	D3D12_FEATURE_DATA_ROOT_SIGNATURE rs_feature_data = {};
	rs_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rs_feature_data, sizeof(rs_feature_data)))) {
		rs_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_ROOT_SIGNATURE_FLAGS rs_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rs_desc;
	rs_desc.Init_1_1(_countof(root_parameters), root_parameters, num_sampler_descriptors, sampler_descriptors, rs_flags);

	ComPtr<ID3DBlob> signature, error;
	HRESULT res = D3DX12SerializeVersionedRootSignature(
		&rs_desc,
		rs_feature_data.HighestVersion,
		&signature,
		&error
	);

	if (FAILED(res)) {
		OutputDebugStringA((char *) error->GetBufferPointer());
		THROW_IF_FAILED(res);
	}

	THROW_IF_FAILED(
		device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&root_signature)
		)
	)
}

std::filesystem::path cg::renderer::dx12_renderer::get_shader_path()
{
	return settings->shader_path;
}

ComPtr<ID3DBlob> cg::renderer::dx12_renderer::compile_shader(const std::string& entrypoint, const std::string& target)
{
	ComPtr<ID3DBlob> shader, error;
	UINT compile_flags = 0;
#ifdef _DEBUG
	compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT res = D3DCompileFromFile(
		get_shader_path().wstring().c_str(),
		nullptr,
		nullptr,
		entrypoint.c_str(),
		target.c_str(),
		compile_flags,
		0,
		&shader,
		&error
	);

	if (FAILED(res)) {
		OutputDebugStringA((char *) error->GetBufferPointer());
		THROW_IF_FAILED(res);
	}

	return shader;
}

void cg::renderer::dx12_renderer::create_pso()
{
	ComPtr<ID3DBlob> vertex_shader = compile_shader("VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixel_shader = compile_shader("PSMain", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC input_descs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.InputLayout = { input_descs, _countof(input_descs) };
	desc.pRootSignature = root_signature.Get();
	desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
	desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.FrontCounterClockwise = TRUE;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;

	THROW_IF_FAILED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline_state)));
}

void cg::renderer::dx12_renderer::create_resource_on_upload_heap(ComPtr<ID3D12Resource>& resource, UINT size, const std::wstring& name)
{
	THROW_IF_FAILED(
		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)
	));
	if (!name.empty()) {
		resource->SetName(name.c_str());
	}
}

void cg::renderer::dx12_renderer::create_resource_on_default_heap(ComPtr<ID3D12Resource>& resource, UINT size, const std::wstring& name, D3D12_RESOURCE_DESC* resource_descriptor)
{
}

void cg::renderer::dx12_renderer::copy_data(const void* buffer_data, UINT buffer_size, ComPtr<ID3D12Resource>& destination_resource)
{
	UINT8 *buffer_data_begin;
	CD3DX12_RANGE read_range(0, 0);
	THROW_IF_FAILED(destination_resource->Map(0, &read_range, reinterpret_cast<void **>(&buffer_data_begin)));
	memcpy(buffer_data_begin, buffer_data, buffer_size);
	destination_resource->Unmap(0, 0);
}

void cg::renderer::dx12_renderer::copy_data(const void* buffer_data, const UINT buffer_size, ComPtr<ID3D12Resource>& destination_resource, ComPtr<ID3D12Resource>& intermediate_resource, D3D12_RESOURCE_STATES state_after, int row_pitch, int slice_pitch)
{
}

D3D12_VERTEX_BUFFER_VIEW cg::renderer::dx12_renderer::create_vertex_buffer_view(
	const ComPtr<ID3D12Resource>& vertex_buffer,
	const UINT vertex_buffer_size
)
{
	D3D12_VERTEX_BUFFER_VIEW view = {};
	view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	view.StrideInBytes = sizeof(vertex);
	view.SizeInBytes = vertex_buffer_size;
	return view;
}

D3D12_INDEX_BUFFER_VIEW cg::renderer::dx12_renderer::create_index_buffer_view(
	const ComPtr<ID3D12Resource>& index_buffer,
	const UINT index_buffer_size
)
{
	D3D12_INDEX_BUFFER_VIEW view = {};
	view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	view.SizeInBytes = index_buffer_size;
	view.Format = DXGI_FORMAT_R32_UINT;
	return view;
}

void cg::renderer::dx12_renderer::create_shader_resource_view(const ComPtr<ID3D12Resource>& texture, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handler)
{
}

void cg::renderer::dx12_renderer::create_constant_buffer_view(const ComPtr<ID3D12Resource>& buffer, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handler)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = buffer->GetGPUVirtualAddress();
	desc.SizeInBytes = ((sizeof(cb) + 255) & ~255);
	device->CreateConstantBufferView(&desc, cpu_handler);
}

void cg::renderer::dx12_renderer::load_assets()
{
	create_root_signature(nullptr, 0);
	create_pso();
	// TODO Lab: 3.06 Create command allocators and a command list

	size_t shape_num = model->get_index_buffers().size();
	vertex_buffers.resize(shape_num);
	vertex_buffer_views.resize(shape_num);
	index_buffers.resize(shape_num);
	index_buffer_views.resize(shape_num);

	for (size_t i = 0; i < shape_num; ++i) {
		auto vertex_buffer_data = model->get_vertex_buffers()[i];
		const UINT vertex_buffer_size = UINT(vertex_buffer_data->size_bytes());
		std::wstring vertex_buffer_name(L"Vertex buffer ");
		vertex_buffer_name += std::to_wstring(i);

		create_resource_on_upload_heap(vertex_buffers[i], vertex_buffer_size, vertex_buffer_name);
		copy_data(vertex_buffer_data->get_data(), vertex_buffer_size, vertex_buffers[i]);
		vertex_buffer_views[i] = create_vertex_buffer_view(vertex_buffers[i], vertex_buffer_size);
		
		auto index_buffer_data = model->get_index_buffers()[i];
		const UINT index_buffer_size = UINT(index_buffer_data->size_bytes());
		std::wstring index_buffer_name(L"Index buffer ");
		index_buffer_name += std::to_wstring(i);

		create_resource_on_upload_heap(index_buffers[i], index_buffer_size, index_buffer_name);
		copy_data(index_buffer_data->get_data(), index_buffer_size, index_buffers[i]);
		index_buffer_views[i] = create_index_buffer_view(index_buffers[i], index_buffer_size);
	}

	std::wstring const_buffer_name(L"Constant buffer");
	create_resource_on_upload_heap(constant_buffer, 64*1024, const_buffer_name);
	copy_data(&cb, sizeof(cb), constant_buffer);

	CD3DX12_RANGE read_range(0, 0);
	THROW_IF_FAILED(constant_buffer->Map(0, &read_range, reinterpret_cast<void **>(&constant_buffer_data_begin)));

	cbv_srv_heap.create_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	create_constant_buffer_view(constant_buffer, cbv_srv_heap.get_cpu_descriptor_handle(0));


	// TODO Lab: 3.07 Create a fence and fence event
}


void cg::renderer::dx12_renderer::populate_command_list()
{
	// TODO Lab: 3.06 Implement `populate_command_list` method
}


void cg::renderer::dx12_renderer::move_to_next_frame()
{
	// TODO Lab: 3.07 Implement `move_to_next_frame` method
}

void cg::renderer::dx12_renderer::wait_for_gpu()
{
	// TODO Lab: 3.07 Implement `wait_for_gpu` method
}


void cg::renderer::descriptor_heap::create_heap(ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT number, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = number;
	desc.Type = type;

	THROW_IF_FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
	descriptor_size = device->GetDescriptorHandleIncrementSize(type);
}

D3D12_CPU_DESCRIPTOR_HANDLE cg::renderer::descriptor_heap::get_cpu_descriptor_handle(UINT index) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		heap->GetCPUDescriptorHandleForHeapStart(),
		INT(index),
		descriptor_size
	);
}

D3D12_GPU_DESCRIPTOR_HANDLE cg::renderer::descriptor_heap::get_gpu_descriptor_handle(UINT index) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		heap->GetGPUDescriptorHandleForHeapStart(),
		INT(index),
		descriptor_size
	);
}
ID3D12DescriptorHeap* cg::renderer::descriptor_heap::get() const
{
	return heap.Get();
}
