#include "fabber_io_newimage.h"

#include "dataset.h"
#include "easylog.h"

#include "newimage/newimage.h"
#include "newimage/newimageio.h"
#include "newmat.h"

#include <string>
#include <vector>
#include <ostream>

using namespace std;
using namespace NEWIMAGE;
using NEWMAT::Matrix;

static void DumpVolumeInfo(const volume4D<float>& info, ostream& out)
{
	out << "FabberIoNewimage::Dimensions: x=" << info.xsize() << ", y=" << info.ysize() << ", z=" << info.zsize()
			<< ", vols=" << info.tsize() << endl;
	out << "FabberIoNewimage::Voxel size: x=" << info.xdim() << "mm, y=" << info.ydim() << "mm, z=" << info.zdim()
			<< "mm, TR=" << info.tdim() << " sec\n";
	out << "FabberIoNewimage::Intents: " << info.intent_code() << ", " << info.intent_param(1) << ", "
			<< info.intent_param(2) << ", " << info.intent_param(3) << endl;
}

static void DumpVolumeInfo(const volume<float>& info, ostream& out)
{
	out << "FabberIoNewimage::Dimensions: x=" << info.xsize() << ", y=" << info.ysize() << ", z=" << info.zsize()
			<< ", vols=1" << endl;
	out << "FabberIoNewimage::Voxel size: x=" << info.xdim() << "mm, y=" << info.ydim() << "mm, z=" << info.zdim()
			<< "mm, TR=1" << " sec\n";
	out << "FabberIoNewimage::Intents: " << info.intent_code() << ", " << info.intent_param(1) << ", "
			<< info.intent_param(2) << ", " << info.intent_param(3) << endl;
}

FabberIoNewimage::FabberIoNewimage() :
		m_have_mask(false)
{
}

void FabberIoNewimage::Initialize(FabberRunData &rundata)
{
	FabberIoMemory::Initialize(rundata);

	// Output files to same dir as log if configured, otherwise to current dir
	m_outdir = "";
	if (m_log) m_log->GetOutputDirectory();
	if (m_outdir == "") m_outdir=".";

	string mask_filename = rundata.GetStringDefault("mask", "");
	if (mask_filename != "")
	{
		LOG_ERR("FabberIoNewimage::Loading mask data from '" + mask_filename << "'" << endl);
		read_volume(m_mask, mask_filename);
		m_mask.binarise(1e-16, m_mask.max() + 1, exclusive);
		DumpVolumeInfo(m_mask, LOG);
		m_have_mask = true;
		SetVoxelCoordsFromExtent(m_mask.xsize(), m_mask.ysize(), m_mask.zsize());
	}
	else
	{
		// Make sure the coords are loaded from the main data even if we don't
		// have a mask, and that the reference volume is initialized
		rundata.GetMainVoxelData();
	}
}

const Matrix &FabberIoNewimage::GetVoxelData(std::string filename)
{
	try
	{
		return FabberIoMemory::GetVoxelData(filename);
	} catch (DataNotFound &e)
	{
		// Load the data file using Newimage library
		// FIXME should check for presence of file before trying to load.
		LOG << "FabberIoNewimage::Loading data from '" + filename << "'" << endl;
		if (!fsl_imageexists(filename))
		{
			throw DataNotFound(filename);
		}

		volume4D<float> vol;
		try
		{
			read_volume4D(vol, filename);
			if (!m_have_mask)
			{
				// We need a mask volume so that when we save we can make sure
				// the image properties are set consistently with the source data
				m_mask = vol[0];
				m_mask = 1;
				m_have_mask = true;
			}
		} catch (...)
		{
			throw DataLoadError(filename);
		}
		DumpVolumeInfo(vol, LOG);

		if (!m_have_coords)
			SetVoxelCoordsFromExtent(vol.xsize(), vol.ysize(), vol.zsize());

		try
		{
			if (m_have_mask)
			{
				LOG << "FabberIoNewimage::Applying mask to data..." << endl;
				m_voxel_data[filename] = vol.matrix(m_mask);
			}
			else
			{
				m_voxel_data[filename] = vol.matrix();
			}
		} catch (exception &e)
		{
			LOG << "*** NEWMAT error while applying mask... Most likely a dimension mismatch. ***\n";
			throw e;
		}

		return m_voxel_data[filename];
	}
}

void FabberIoNewimage::SaveVoxelData(NEWMAT::Matrix &data, string filename, VoxelDataType data_type)
{
	LOG << "FabberIoNewimage::Saving to nifti: " << filename << endl;
	int nifti_intent_code;
	switch (data_type)
	{
	case VDT_MVN:
		nifti_intent_code = NIFTI_INTENT_SYMMATRIX;
		break;
	default:
		nifti_intent_code = NIFTI_INTENT_NONE;
	}

	int data_size = data.Nrows();
	volume4D<float> output(m_extent[0], m_extent[1], m_extent[2], data_size);
	if (m_have_mask)
	{
		output.setmatrix(data, m_mask);
	}
	else
	{
		output.setmatrix(data);
	}

	output.set_intent(nifti_intent_code, 0, 0, 0);
	output.setDisplayMaximumMinimum(output.max(), output.min());

	if (m_outdir != "")
		filename = m_outdir + "/" + filename;
	save_volume4D(output, filename);
}

void FabberIoNewimage::SetVoxelCoordsFromExtent(int nx, int ny, int nz)
{
	LOG << "FabberIoNewimage::Setting voxel coordinates from extent" << endl;

	volume4D<float> coordvol(nx, ny, nz, 3);
	for (int i = 0; i < nx; i++)
	{
		for (int j = 0; j < ny; j++)
		{
			for (int k = 0; k < nz; k++)
			{
				ColumnVector vcoord(3);
				vcoord << i << j << k;
				coordvol.setvoxelts(vcoord, i, j, k);
			}
		}
	}

	if (m_have_mask)
	{
		SetVoxelCoords(coordvol.matrix(m_mask));
	}
	else
	{
		SetVoxelCoords(coordvol.matrix());
	}

	// Override the setting of extent from co-ordinates since
	// the presence of a mask may mean that the co-ords do not extent over
	// the full range from the NIFTI file. But we do want this full range
	// reflected in our output data!
	m_extent[0] = nx;
	m_extent[1] = ny;
	m_extent[2] = nz;

	m_have_coords = true;
}
