/**
 * \file hardwareSensorCameraFirewire.hpp
 *
 * Header file for getting data from a firewire camera
 *
 * \date 18/06/2010
 * \author croussil
 *
 * \ingroup rtslam
 */

#ifndef HARDWARE_SENSOR_CAMERA_FIREWIRE_HPP_
#define HARDWARE_SENSOR_CAMERA_FIREWIRE_HPP_

#include <jafarConfig.h>
#include <image/Image.hpp>
#include <kernel/threads.hpp>

#ifdef HAVE_VIAM
#include <viam/viamlib.h>
#include <viam/viamcv.h>
#endif

#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "rtslam/hardwareSensorAbstract.hpp"
#include "rtslam/rawImage.hpp"


namespace jafar {
namespace rtslam {
namespace hardware {

/**
This class allows to get images from firewire with non blocking procedure,
using triple-buffering.
*/
class HardwareSensorCameraFirewire: public HardwareSensorAbstract
{
	private:
#ifdef HAVE_VIAM
		viam_bank_t bank;
		viam_handle_t handle;
#endif
		boost::mutex mutex_data;
		
		int buff_in_use;
		int buff_ready;
		int buff_write;
		int image_count;
		IplImage* buffer[3];
		raw_ptr_t bufferPtr[3];
		rawimage_ptr_t bufferSpecPtr[3];
		double realFreq;
		bool no_more_data;
		unsigned first_index;
		int found_first; /// 0 = not found, 1 = found pgm, 2 = found png
		
		int mode;
		kernel::VariableCondition<int> index;
		std::string dump_path;
		
		boost::thread *preloadTask_thread;
		void preloadTask(void);
	
#ifdef HAVE_VIAM
		cv::Size viamSize_to_size(viam_hwsize_t hwsize);
		double viamFreq_to_freq(viam_hwfps_t hwfreq);
		
		viam_hwsize_t size_to_viamSize(cv::Size size);
		viam_hwfps_t freq_to_viamFreq(double freq);
		viam_hwformat_t format_to_viamFormat(int format, int depth);
		viam_hwtrigger_t trigger_to_viamTrigger(int trigger);
#endif
	
		void init(int mode, std::string dump_path, cv::Size imgSize);
#ifdef HAVE_VIAM
		void init(const std::string &camera_id, viam_hwmode_t &hwmode, double shutter, int mode, std::string dump_path);
#endif
	public:
		
#ifdef HAVE_VIAM
		/**
		@param camera_id the Firewire camera id (0x....)
		@param hwmode the hardware mode
		@param mode 0 = normal, 1 = dump used images, 2 = from dumped images
		@param dump_path the path where the images are saved/read... Use a ram disk !!!
		*/
		HardwareSensorCameraFirewire(boost::condition_variable &rawdata_condition, boost::mutex &rawdata_mutex, const std::string &camera_id, cv::Size size, int format, int depth, double freq, int trigger, double shutter, int mode = 0, std::string dump_path = ".");
#endif
		/**
		Same as before but assumes that mode=2, and doesn't need a camera
		*/
		HardwareSensorCameraFirewire(boost::condition_variable &rawdata_condition, boost::mutex &rawdata_mutex, cv::Size imgSize, std::string dump_path = ".");
		
		~HardwareSensorCameraFirewire();
		
		virtual int acquireRaw(raw_ptr_t &rawPtr);

		double getFreq() { return realFreq; }
};


}}}

#endif
