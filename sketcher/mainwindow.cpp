#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QImage>

#include "gpu_image.h"
#include "gpu_color.h"
#include "gpu_st.h"
#include "gpu_gauss.h"
#include "gpu_noise.h"
#include "gpu_stgauss2.h"
#include "gpu_dog.h"
#include "gpu_dog2.h"
#include "gpu_bilateral.h"
#include "gpu_shuffle.h"

#include "imageutil.h"
#include "imageutil.cpp"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->show();

    qApp->processEvents();

    QImage input( QFileDialog::getOpenFileName(0, "Open Image", "", "Image Files (*.png *.jpg *.bmp)") );

    ui->imagelabel->setPixmap( QPixmap::fromImage( process(input) ) );
}

MainWindow::~MainWindow()
{
    delete ui;
}

QImage MainWindow::process(QImage qsrc)
{
    /// Parameters:
    double sigma_c = 2.28;
    double precision_sigma_c = sqrt(-2*log(0.05));
    double bf_alpha = 0;
    double step_m = 1.0;
    double sigma_dg = 3;
    double sigma_rg = 4.25;
    double sigma_e = 1.4;
    double sigma_m = 1.0;
    double sigma_a = 1.0;
    double precision_e = 3;
    double precision_m = 2;
    double dog_k = 1.6;
    double dog_eps_p = 79.5;
    double dog_phi_p = 0.017;
    double dog_p = 21.7;

    gpu_image<float4> src = gpu_image_from_qimage<float4>(qsrc);
    //if (input_gamma == "linear-rgb")
    {
        src = gpu_linear2srgb(src);
    }

    gpu_image<float4> lab = gpu_rgb2lab(src);

    gpu_image<float4> st;

    //if (st_type == "scharr-rgb")
    {
        st = gpu_st_scharr(src);
        st = gpu_gauss_filter_xy(st, sigma_c, precision_sigma_c);
    }

    gpu_image<float4> lfm = gpu_st_lfm(st, bf_alpha);

    gpu_image<float> flow;

    gpu_image<float> m_noise = gpu_noise_random(st.w(), st.h(), -1, 2);

    flow = m_noise;
    flow = gpu_stgauss2_filter(flow, st, 6, 22.5f, false, true, true, 2, 1.0f, 2);
    flow = gpu_stgauss2_filter(flow, st, 1, 22.5, false, true, true, 2, 1.0f, 2 );

    gpu_image<float4> img = lab;
    gpu_image<float4> Ie = img;
    gpu_image<float4> Ia = img;

    //if (enable_bf)
    {
        /*int N = std::max(n_e, n_a);
        for (int i = 0; i < N; ++i) {
            if (filter_type == "oa") {
                img = gpu_oabf_1d(img, lfm, sigma_dg, sigma_rg, false, precision_g);
                img = gpu_oabf_1d(img, lfm, sigma_dt, sigma_rt, true, precision_t);
            } else if (filter_type == "xy") {*/
                img = gpu_bilateral_filter_xy(img, sigma_dg, sigma_rg);
            /*} else if (filter_type == "fbl") {
                img = gpu_oabf_1d(img, lfm, sigma_dg, sigma_rg, false, precision_g);
                img = gpu_stbf2_filter(img, st, sigma_dt, sigma_rt, precision_t, 90.0f, false, true, true, 2, 1);
            } else {
                img = gpu_bilateral_filter(img, sigma_dg, sigma_rg);
            }
            if (i == (n_e - 1)) Ie = img;
            if (i == (n_a - 1)) Ia = img;
        }*/
    }

    gpu_image<float> L;
    //if (output != "fill")
    {
        L = gpu_shuffle(Ie, 0);
        //if (dog_type == "flow-based")
        {
            L = gpu_gradient_dog2( L, lfm, sigma_e, dog_k, dog_p, precision_e );

            L = gpu_stgauss2_filter(L, st, sigma_m, 90, false, false, false, 1, step_m, precision_m);
        }

        {
            float eps, phi;
            {
                eps = dog_eps_p;
                phi = dog_phi_p;
            }

            L = gpu_dog_threshold_smoothstep(L, eps, phi);
        }
    }

    img = gpu_l2rgb(gpu_mul(L,100));

    img = gpu_stgauss2_filter( img, st, sigma_a, 90, false, false, false, 2, 1.0f, 2);

    QImage m_result[3];

    m_result[0] = gpu_image_to_qimage(img);
    m_result[1] = qsrc;
    m_result[2] = gpu_image_to_qimage(flow);

    return m_result[0];
}
