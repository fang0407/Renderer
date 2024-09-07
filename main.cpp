#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

// 顶点着色器源码
const char *gVertexShaderSource = "#version 330 core\n"
                                  "layout (location = 0) in vec3 aPos;\n"
                                  "layout (location = 1) in vec2 aTexCoords;\n"
                                  "out vec2 TexCoords;\n"
                                  "void main()\n"
                                  "{\n"
                                  "   gl_Position = vec4(aPos, 1.0);\n"
                                  "   TexCoords = aTexCoords;\n"
                                  "}\0";

// 片段着色器源码
const char *gFragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoords;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(texture1, TexCoords);\n"
    "}\0";

void FramebufferSizeCob(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {
  // 打开视频文件
  AVFormatContext *formatContext = avformat_alloc_context();
  if (avformat_open_input(&formatContext, "input.mp4", NULL, NULL) != 0) {
    std::cerr << "Could not open video file" << std::endl;
    return -1;
  }

  if (avformat_find_stream_info(formatContext, NULL) < 0) {
    std::cerr << "Could not find stream information" << std::endl;
    return -1;
  }

  // 找到视频流
  int videoStreamIndex = -1;
  for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      break;
    }
  }

  if (videoStreamIndex == -1) {
    std::cerr << "Could not find video stream" << std::endl;
    return -1;
  }

  // 获取视频流的解码器
  AVCodecParameters *codecParameters =
      formatContext->streams[videoStreamIndex]->codecpar;
  AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
  if (!codec) {
    std::cerr << "Could not find decoder" << std::endl;
    return -1;
  }

  AVCodecContext *codecContext = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(codecContext, codecParameters);
  if (avcodec_open2(codecContext, codec, NULL) < 0) {
    std::cerr << "Could not open codec" << std::endl;
    return -1;
  }

  // 初始化GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // 创建窗口对象
  GLFWwindow *window = glfwCreateWindow(800, 600, "Video Playback", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  // 通知GLFW将我们窗口的上下文设置为当前线程的主上下文了
  glfwMakeContextCurrent(window);

  // glewInit函数会初始化GLEW并尝试加载所有可用的OpenGL扩展
  glewExperimental = GL_TRUE;
  glewInit();

  // 设置OpenGL渲染窗口的尺寸大小
  glViewport(0, 0, 800, 600);

  // 每当窗口调整大小的时候调用FramebufferSizeCob
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCob);

  ////////////////////////////////////////////////
  // 创建顶点着色器
  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  // 将源码附加到顶点着色器上
  glShaderSource(vertex_shader, 1, &gVertexShaderSource, NULL);
  // 编译
  glCompileShader(vertex_shader);
  // 检查
  int success;
  char infoLog[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    return -1;
  }

  // 创建片段着色器
  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  // 将源码附加到片段着色器上
  glShaderSource(fragment_shader, 1, &gFragmentShaderSource, NULL);
  // 编译
  glCompileShader(fragment_shader);
  // 检查
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    return -1;
  }

  // 链接着色器到程序
  unsigned int shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    return -1;
  }

  // 在把着色器对象链接到程序对象以后，记得删除着色器对象
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // 设置顶点数据和缓冲
  float vertices[] = {
      // 加载的纹理位置和顶点位置相反, 因为设备一般左上角为原点
      // 因此需要我们在使用纹理坐标系时，对纹理先进性第一步处理，即上下颠倒一次
      // 位置              // 纹理坐标
      -1.0f, 1.0f,  0.0f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
      1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f};
  unsigned int indices[] = {
      0, 1, 2, // 第一个三角形
      2, 3, 0  // 第二个三角形
  };

  unsigned int VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  // 绑定随后的顶点属性调用都会储存在这个VAO中
  glBindVertexArray(VAO);

  // 顶点数组复制到缓冲中供OpenGL使用
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // 复制索引数组到一个索引缓冲中，供OpenGL使用
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // 设置顶点属性指针
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // 创建纹理
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 一个纹理的默认纹理单元是0，它是默认的激活纹理单元，这里可以不用设置
  // 设置每个采样器的方式告诉OpenGL每个着色器采样器属于哪个纹理单元，只需要设置一次即可
  glUseProgram(shader_program);
  glUniform1i(glGetUniformLocation(shader_program, "texture1"), 0);

  // 渲染循环
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();
  AVFrame *rgb_frame = av_frame_alloc();
  int width = codecContext->width;
  int height = codecContext->height;
  int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
  uint8_t *buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
  av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                       AV_PIX_FMT_RGB24, width, height, 1);
  struct SwsContext *sws_ctx =
      sws_getContext(width, height, codecContext->pix_fmt, width, height,
                     AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  // 每次循环的开始前检查一次GLFW是否被要求退出，如果是的话，该函数返回true，渲染循环将停止运行，之后我们就可以关闭应用程序
  while (!glfwWindowShouldClose(window) &&
         av_read_frame(formatContext, &packet) >= 0) {
    if (packet.stream_index == videoStreamIndex) {
      AVFrame *dec_frame = av_frame_alloc();
      if (avcodec_send_packet(codecContext, &packet) == 0) {
        while (avcodec_receive_frame(codecContext, dec_frame) == 0) {
          // 自定义的颜色清空屏幕
          glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
          // 只清空颜色缓冲
          glClear(GL_COLOR_BUFFER_BIT);

          // YUV420->RGB, 存储到buffer中
          sws_scale(sws_ctx, (const uint8_t *const *)dec_frame->data,
                    dec_frame->linesize, 0, height, rgb_frame->data,
                    rgb_frame->linesize);

          // 在绑定纹理之前先激活纹理单元，这里可以不用设置，默认激活的单元0
          glActiveTexture(GL_TEXTURE0);
          // 绑定纹理
          glBindTexture(GL_TEXTURE_2D, texture);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                       GL_UNSIGNED_BYTE, buffer);

          // 激活这个程序对象
          glUseProgram(shader_program);
          glBindVertexArray(VAO);
          // 使用当前绑定的索引缓冲对象中的索引进行绘制
          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

          // 函数会交换颜色缓冲（它是一个储存着GLFW窗口每一个像素颜色值的大缓冲），它在这一迭代中被用来绘制，并且将会作为输出显示在屏幕上
          glfwSwapBuffers(window);
          // 函数检查有没有触发什么事件（比如键盘输入、鼠标移动等）、更新窗口状态，并调用对应的回调函数（可以通过回调方法手动设置）
          glfwPollEvents();
        }
      }
      av_frame_free(&dec_frame);
      av_packet_unref(&packet);
    }
  }

  // 释放资源
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shader_program);
  glfwTerminate();

  return 0;
}
