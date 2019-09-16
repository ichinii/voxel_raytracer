#include <iostream>
#include <numeric>
#include <algorithm>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shader.hpp"
#include <thread>

using namespace std::chrono_literals;

template <typename CharT, typename Traits>
auto& operator << (std::basic_ostream<CharT, Traits>& os, glm::vec3 vec)
{
	return os << '(' << vec.x << '|' << vec.y << '|' << vec.z << ')';
}

constexpr auto size = glm::uvec3(1, 1, 1) * 16u;
constexpr auto length = size.x * size.y * size.z;
using cube_t = unsigned char;
using cubes_t = cube_t[length];
// using camera_t = struct { glm::vec3 pos, dir; glm::uvec2 size; float fov; };

int main()
{
	std::srand(std::time(0));

	if (!glfwInit()) return 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	auto window_size = glm::ivec2{1280, 720};
	auto window = glfwCreateWindow(window_size.x, window_size.y, "voxel_renderer", nullptr, nullptr);
	glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, [[maybe_unused]] int scancode, [[maybe_unused]] int action, int mods) {
		if (!mods && key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
	});
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (glewInit() != GLEW_OK) return 0;
	glEnable(GL_TEXTURE_3D);
	glClearColor(0.5, 0, 0, 1);

	// std::this_thread::sleep_for(1s);
	auto display_program = createProgram({{GL_VERTEX_SHADER, "res/vertex.glsl"}, {GL_FRAGMENT_SHADER, "res/fragment.glsl"}});
	auto voxel_program = createProgram({{GL_COMPUTE_SHADER, "res/voxel_compute.glsl"}});

	glUseProgram(voxel_program);

	const auto voxel_tex_size = glm::uvec2(1024, 1024);
	GLuint voxel_tex_out;
	glGenTextures(1, &voxel_tex_out);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, voxel_tex_out);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, voxel_tex_size.x, voxel_tex_size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, voxel_tex_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	GLuint voxel_cubes_tex;
	glGenTextures(1, &voxel_cubes_tex);
	glBindTexture(GL_TEXTURE_3D, voxel_cubes_tex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	cubes_t cubes;
	std::generate_n(cubes, length, [] { return std::rand() % 8 == 0 ? std::rand() % 192 + 64 : 0; });
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, size.x, size.y, size.z, 0, GL_RED, GL_UNSIGNED_BYTE, cubes);

	// int work_grp_cnt[3];
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	// printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
	// int work_grp_size[3];
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	// printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);
	// int work_grp_inv;
	// glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	// printf("max local work group invocations %i\n", work_grp_inv);
	
	glUseProgram(display_program);
	enum { vertex_position, vertex_uv };
	GLuint vao;
	GLuint vbos[2];
	glCreateVertexArrays(1, &vao);
	glGenBuffers(2, vbos);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(vertex_position);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vertex_position]);
	glm::vec2 vertex_positions[] = { {-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof (vertex_positions), vertex_positions, GL_STATIC_DRAW);
	glVertexAttribPointer(vertex_position, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(vertex_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vertex_uv]);
	glm::vec2 vertex_uvs[] = { {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof (vertex_uvs), vertex_uvs, GL_STATIC_DRAW);
	glVertexAttribPointer(vertex_uv, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindVertexArray(0);
	glUseProgram(0);

	using clock = std::chrono::steady_clock;
	auto start_time = clock::now();
	auto last_time = start_time;
	auto last_second = start_time;
	auto time_ms_sum = 0ms;
	auto time_us_sum = 0us;
	auto frames_per_seconds_count = 0;

	while (!glfwWindowShouldClose(window)) {
		auto time_before = clock::now();
		[[maybe_unused]] auto delta_time = time_before - last_time;
		last_time = time_before;
		auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_before - start_time);

		{ // launch compute shaders and draw to image
			glUseProgram(voxel_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, voxel_cubes_tex);
			auto camera_pos = glm::vec3 { glm::sin(elapsed_time.count() / 5000.f) * size.x * 1.5f, glm::cos(elapsed_time.count() / 5000.f) * size.y , size.z * 2};
			// auto camera_pos = glm::vec3(size) * glm::vec3(1, 2, 1);
			auto camera_view_mat = glm::inverse(glm::lookAt(camera_pos, {0, 0, 0}, {0, 1, 0}));
			glUniformMatrix4fv(glGetUniformLocation(voxel_program, "camera_view_mat"), 1, GL_FALSE, &camera_view_mat[0][0]);
			glDispatchCompute((GLuint)voxel_tex_size.x, (GLuint)voxel_tex_size.y, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		{ // present image to screen
			glUseProgram(display_program);
			glUniform2i(glGetUniformLocation(display_program, "viewport_size"), window_size.x, window_size.y);
			glUniform2i(glGetUniformLocation(display_program, "tex_size"), voxel_tex_size.x, voxel_tex_size.y);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindVertexArray(vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, voxel_tex_out);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
			glfwSwapBuffers(window);
			glBindVertexArray(0);
		}

		{ // timing and logging
			++frames_per_seconds_count;
			auto time_after = clock::now();
			auto time_diff = time_after - time_before;
			time_ms_sum += std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);
			time_us_sum += std::chrono::duration_cast<std::chrono::microseconds>(time_diff);
			if (std::chrono::duration_cast<std::chrono::seconds>(time_after - last_second).count() >= 1) {
				time_ms_sum /= frames_per_seconds_count;
				time_us_sum /= frames_per_seconds_count;
				std::cout << "avg dur per frame: " << time_ms_sum.count() << "ms ( "
					<< time_us_sum.count() << "us ) "
					<< frames_per_seconds_count << "fps" << std::endl;
				time_ms_sum = 0ms;
				time_us_sum = 0us;
				frames_per_seconds_count = 0;
				last_second = time_after;
			}
		}

		glfwPollEvents();
		glfwGetWindowSize(window, &window_size.x, &window_size.y);
		glViewport(0, 0, window_size.x, window_size.y);
	}

	glfwTerminate();

	return 0;
}
