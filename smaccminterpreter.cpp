#include "smaccminterpreter.hpp"

SmaccmInterpreter::SmaccmInterpreter() : 
  socket(io_service, udp::endpoint(udp::v4(), 1337)){

}

int SmaccmInterpreter::init(){

  boost::array<char, 1> recv_buf;
  boost::system::error_code error;
  socket.receive_from(boost::asio::buffer(recv_buf),
      remote_endpoint, 0, error);

  if (error && error != boost::asio::error::message_size){
    throw boost::system::system_error(error);
  }


  pImage = new bitmap_image(sentWidth, sentHeight);
  //start the thread that will be used to send frames
  boost::thread frameSenderThread(boost::bind(&SmaccmInterpreter::sendFrame, this));
  PixyInterpreter::init();

 // acceptor = new tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), atoi(argv[1])));
  //socket(io_service);
  //acceptor.accept(socket);

}

void SmaccmInterpreter::sendFrame(){
  boost::system::error_code ignored_error;
  for(;;){
    usleep(50);
    imageMutex.lock();
    //pImage->save_image("output.bmp");
    socket.send_to(boost::asio::buffer(processedPixels, sentWidth*sentHeight*sizeof(uint32_t)),
      remote_endpoint, 0, ignored_error);
    imageMutex.unlock();
  }
}

void SmaccmInterpreter::interpolateBayer(unsigned int width, unsigned int x, unsigned int y, unsigned char *pixel, unsigned int &r, unsigned int &g, unsigned int &b)
{
    if (y&1)
    {
        if (x&1)
        {
            r = *pixel;
            g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
            b = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
        }
        else
        {
            r = (*(pixel-1)+*(pixel+1))>>1;
            g = *pixel;
            b = (*(pixel-width)+*(pixel+width))>>1;
        }
    }
    else
    {
        if (x&1)
        {
            r = (*(pixel-width)+*(pixel+width))>>1;
            g = *pixel;
            b = (*(pixel-1)+*(pixel+1))>>1;
        }
        else
        {
            r = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
            g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
            b = *pixel;
        }
    }
}


int SmaccmInterpreter::renderBA81(uint16_t width, uint16_t height, uint8_t *frame, uint32_t * lines, bitmap_image *pImage)
{
    uint16_t x, y;
    uint32_t *line;
    uint32_t r, g, b;

    imageMutex.lock();
    image_drawer drawer(*pImage);
    // skip first line
    frame += width;

    // don't render top and bottom rows, and left and rightmost columns because of color
    // interpolation

    for (y=1; y<height-1; y++)
    {
        line = (unsigned int *)(lines + (y-1)*width);
        frame++;
        for (x=1; x<width-1; x++, frame++)
        {
            interpolateBayer(width, x, y, frame, r, g, b);
            *line++ = (0x40<<24) | (r<<16) | (g<<8) | (b<<0);
            drawer.pen_color((char)r,(char)g,(char)b);            
            drawer.plot_pixel(x,y);
        }
        frame++;
    }
    imageMutex.unlock();
    return 0;
}

void SmaccmInterpreter::interpret_data(void * chirp_data[])
{
  uint8_t  chirp_message;
  uint32_t chirp_type;
  static int t = 0;

  if (chirp_data[0]) {

    chirp_message = Chirp::getType(chirp_data[0]);

    switch(chirp_message) {
      
      case CRP_TYPE_HINT:
        
        chirp_type = * static_cast<uint32_t *>(chirp_data[0]);
  
        switch(chirp_type) {

          case FOURCC('B', 'A', '8', '1'):
            uint16_t width, height;
            uint32_t frame_len;
            uint8_t * pFrame;
			width = *(uint16_t *)chirp_data[2];
			height = *(uint16_t *)chirp_data[3];
			frame_len = *(uint32_t *)chirp_data[4];
			pFrame = (uint8_t *)chirp_data[5];
			assert(width == sentWidth);
			assert(height == sentHeight);
			assert(frame_len = width*height);
            printf("rendering %d\n", t++);
            renderBA81(width, height, pFrame, processedPixels, pImage);
            //pImage->save_image("output.bmp"); 
            break;
          case FOURCC('C', 'C', 'Q', '1'):
            break;
          case FOURCC('C', 'C', 'B', '1'):
            //interpret_CCB1(chirp_data + 1);
            break;
          case FOURCC('C', 'C', 'B', '2'):
            //interpret_CCB2(chirp_data + 1);
            break;
          case FOURCC('C', 'M', 'V', '1'):
            break;
          default:
            printf("libpixy: Chirp hint [%u] not recognized.\n", chirp_type);
            break;
        }

        break;

      case CRP_HSTRING:

        break;
      
      default:
       
       fprintf(stderr, "libpixy: Unknown message received from Pixy: [%u]\n", chirp_message);
       break;
    }
  } 
}
