#ifndef _PANGOLIN_BASE_H_
#define _PANGOLIN_BASE_H_

#include <glog/logging.h>
#include <iostream>
#include <memory.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <thread>

#include <pangolin/pangolin.h>

struct CustomType {
    CustomType()
        : x(0)
        , y(0.0f)
    {
    }

    CustomType(int x, float y, std::string z)
        : x(x)
        , y(y)
        , z(z)
    {
    }

    int x;
    float y;
    std::string z;
};
// TODO: how to merget into class
std::ostream& operator<<(std::ostream& os, const CustomType& o)
{
    os << o.x << " " << o.y << " " << o.z;
    return os;
}

std::istream& operator>>(std::istream& is, CustomType& o)
{
    is >> o.x;
    is >> o.y;
    is >> o.z;
    return is;
}

class PangolinBase {
public:
    typedef std::shared_ptr<PangolinBase> Ptr;

    PangolinBase()
    {
        std::cout << "Init Pangolin" << std::endl;
        LOG(INFO) << "Init Pangolin class";

        // window
        window_name = "Main";
        w = 640;
        h = 480;
        UI_WIDTH = 180;

        //image
        imageWidth = 64;
        imageHeight = 48;

        setup();
        // Thread
        render_loop = std::thread(std::thread(&PangolinBase::run, this));
    }
    ~PangolinBase()
    {
        delete[] imageArray;
        render_loop.join();
    }
    void SampleMethod()
    {
        std::cout << "You typed ctrl-r or pushed reset" << std::endl;
    }

    void setup()
    { // create a window and bind its context to the main thread
        pangolin::CreateWindowAndBind(window_name, w, h);

        // enable depth
        glEnable(GL_DEPTH_TEST);

        // unset the current context from the main thread
        pangolin::GetBoundWindow()->RemoveCurrent();
    }

    void guiInit()
    {
        // Load configuration data
        pangolin::ParseVarsFile("../app.cfg");

        a_button = new pangolin::Var<bool>("ui.a_Button", false, false);
        a_checkbox = new pangolin::Var<bool>("ui.A_Checkbox", false, true);
        a_double = new pangolin::Var<double>("ui.A_Double", 3.25, 0, 5);
        an_int = new pangolin::Var<int>("ui.An_Int", 2, 0, 5);
        a_double_log = new pangolin::Var<double>("ui.Log_scale var", 3, 1, 1E4, true);
        an_int_no_input = new pangolin::Var<int>("ui.An_Int_No_Input", 2);

        save_window = new pangolin::Var<bool>("ui.Save_Window", false, false);
        save_cube = new pangolin::Var<bool>("ui.Save_Cube", false, false);
        record_cube = new pangolin::Var<bool>("ui.Record_Cube", false, false);

        // any_type = new pangolin::Var<CustomType>("ui.Some_Type", CustomType(0, 1.2f, std::string("Hello")));

        // event handler
        pangolin::Var<std::function<void(void)>> reset("ui.Reset", std::bind(&PangolinBase::SampleMethod, this));
        pangolin::RegisterKeyPressCallback(pangolin::PANGO_CTRL + 'r', std::bind(&PangolinBase::SampleMethod, this));

        // image
        imageArray = new unsigned char[3 * imageWidth * imageHeight];
        imageTexture = new pangolin::GlTexture(imageWidth, imageHeight, GL_RGB, false, 0, GL_RGB, GL_UNSIGNED_BYTE);
    }

    void guiHandler()
    {
        if (pangolin::Pushed(*a_button))
            std::cout << "You Pushed a button!" << std::endl;

        if (*a_checkbox) {
            // std::cout << "You checked checkbox!" << std::endl;
            *an_int = (int)*a_double;
        }
        if (pangolin::Pushed(*save_window)) {
            std::cout << "Save window" << std::endl;
            pangolin::SaveWindowOnRender("window");
        }

        if (pangolin::Pushed(*save_cube)) {
            std::cout << "Save cube" << std::endl;
            d_cam->SaveOnRender("cube");
        }

        // if (!(*any_type)->z.compare("robot")) {
        // std::cout << "CustomType triggered" << std::endl;
        // *any_type = CustomType(1, 2.3f, "Boogie");
        // }

        if (pangolin::Pushed(*record_cube))
            pangolin::DisplayBase().RecordOnRender("ffmpeg:[fps=50,bps=8388608,unique_filename]//screencap.avi");
    }

    void run()
    {
        LOG(INFO) << "-----------------------------";
        LOG(INFO) << "Pangolin thread running";

        // fetch the context and bind it to this thread
        pangolin::BindToContext(window_name);

        // we manually need to restore the properties of the context
        glEnable(GL_DEPTH_TEST);

        // Define Projection and initial ModelView matrix
        pangolin::OpenGlRenderState s_cam(
            pangolin::ProjectionMatrix(w, h, 420, 420, 320, 240, 0.2, 100),
            pangolin::ModelViewLookAt(-2, 2, -2, 0, 0, 0, pangolin::AxisY));

        // Create Interactive View in window
        // pangolin::Handler3D handler(s_cam);
        // Add named OpenGL viewport to window and provide 3D Handler
        d_cam = &pangolin::Display("cam")
                     .SetBounds(0.0f, 1.0f, pangolin::Attach::Pix(UI_WIDTH), 1.0, -float(w) / float(h))
                     .SetHandler(new pangolin::Handler3D(s_cam));

        //  Show image
        d_image = &pangolin::Display("image")
                       .SetBounds(0.0f, 0.3f, 0.0f, pangolin::Attach::Pix(UI_WIDTH), float(w) / float(h))
                       .SetLock(pangolin::LockLeft, pangolin::LockTop);

        // pangolin::Var<CustomType> any_type("ui.Some_Type", CustomType(0, 1.2f, "Hello"));
        // Add named Panel and bind to variables beginning 'ui'
        // A Panel is just a View with a default layout and input handling
        pangolin::CreatePanel("ui")
            .SetBounds(0.3f, 1.0f, 0.0, pangolin::Attach::Pix(UI_WIDTH));

        // GUI panel
        guiInit();

        // event handler
        pangolin::Var<std::function<void(void)>> reset("ui.Reset", std::bind(&PangolinBase::SampleMethod, this));
        pangolin::RegisterKeyPressCallback(pangolin::PANGO_CTRL + 'r', std::bind(&PangolinBase::SampleMethod, this));

        // setImageData(imageArray, 3 * imageWidth * imageHeight);
        while (!pangolin::ShouldQuit()) {
            // Clear screen and activate view to render into
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            d_cam->Activate(s_cam);

            // Event handler for mouse and keyboard event
            guiHandler();

            // Render OpenGL Cube
            pangolin::glDrawColouredCube();

            //display the image
            // setImageData(imageArray, 3 * imageWidth * imageHeight);
            setRandomImageData(imageArray, 3 * imageWidth * imageHeight);
            imageTexture->Upload(imageArray, GL_RGB, GL_UNSIGNED_BYTE);
            d_image->Activate();
            glColor3f(1.0, 1.0, 1.0);
            imageTexture->RenderToViewport();
            // Swap frames and Process Events
            pangolin::FinishFrame();
        }

        // unset the current context from the main thread
        pangolin::GetBoundWindow()->RemoveCurrent();
    }

    // Random image data
    void setRandomImageData(unsigned char* imageArray, int size)
    {
        for (int i = 0; i < size; i++) {
            imageArray[i] = (unsigned char)(rand() / (RAND_MAX / 255.0));
        }
    }

    // Image data
    void setImageData(unsigned char* imageArray, int size)
    {
        cv::Mat image;
        image = cv::imread("../data/rgb.png");
        if (!image.data) {
            printf("No image data \n");
            return;
        }
        int rows = size / 2;
        int cols = size / rows;
        // std::cout << "rows * cols: " << rows << "*" << cols << std::endl;
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                int w = i * rows + j;
                if (w < size)
                    imageArray[w] = image.at<uchar>(i, j);
            }
    }

private:
    std::string window_name;
    int w;
    int h;
    int UI_WIDTH;

    pangolin::Var<bool>* a_button;
    pangolin::Var<bool>* a_checkbox;
    pangolin::Var<double>* a_double;
    pangolin::Var<int>* an_int;
    pangolin::Var<double>* a_double_log;
    pangolin::Var<int>* an_int_no_input;

    pangolin::Var<bool>* save_window;
    pangolin::Var<bool>* save_cube;
    // pangolin::Var<CustomType>* any_type;

    pangolin::Var<bool>* record_cube;
    pangolin::View* d_cam;
    pangolin::View* d_image;

    // thread
    std::thread render_loop;
    //image
    int imageWidth;
    int imageHeight;
    unsigned char* imageArray;
    pangolin::GlTexture* imageTexture;
};

#endif
