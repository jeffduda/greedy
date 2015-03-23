/*=========================================================================

  Program:   ALFABIS fast image registration
  Language:  C++

  Copyright (c) Paul Yushkevich. All rights reserved.

  This program is part of ALFABIS

  ALFABIS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ALFABIS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ALFABIS.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef MULTICOMPONENTMUTUALINFOIMAGEMETRIC_H
#define MULTICOMPONENTMUTUALINFOIMAGEMETRIC_H

#include "MultiComponentImageMetricBase.h"
#include "itkBarrier.h"
#include <queue>
#include <vector>

/**
 * Default traits for parameterizing the metric filters below.
 */
template <class TReal, class TIndex, unsigned int VDim>
struct DefaultMultiComponentMutualInfoImageMetricTraits
{
  typedef itk::VectorImage<TIndex, VDim> InputImageType;
  typedef itk::Image<TReal, VDim> ScalarImageType;
  typedef itk::Image<itk::CovariantVector<TReal, VDim>, VDim> VectorImageType;

  typedef ScalarImageType MaskImageType;
  typedef VectorImageType DeformationFieldType;
  typedef VectorImageType GradientImageType;
  typedef ScalarImageType MetricImageType;
  typedef itk::MatrixOffsetTransformBase<TReal, VDim, VDim> TransformType;

  typedef TReal RealType;
};


template <class TMetricTraits>
class ITK_EXPORT MultiComponentMutualInfoImageMetric :
    public MultiComponentImageMetricBase<TMetricTraits>
{
public:
  /** Standard class typedefs. */
  typedef MultiComponentMutualInfoImageMetric<TMetricTraits> Self;
  typedef MultiComponentImageMetricBase<TMetricTraits>       Superclass;
  typedef itk::SmartPointer<Self>                            Pointer;
  typedef itk::SmartPointer<const Self>                      ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods) */
  itkTypeMacro( MultiImageOpticalFlowImageFilter, MultiComponentImageMetricBase )

  /** Typedef to describe the output image region type. */
  typedef typename Superclass::OutputImageRegionType         OutputImageRegionType;

  /** Inherit some types from the superclass. */
  typedef typename Superclass::InputImageType                InputImageType;
  typedef typename Superclass::InputPixelType                InputPixelType;
  typedef typename Superclass::InputComponentType            InputComponentType;
  typedef typename Superclass::MetricImageType               MetricImageType;
  typedef typename Superclass::GradientImageType             GradientImageType;
  typedef typename Superclass::MetricPixelType               MetricPixelType;
  typedef typename Superclass::GradientPixelType             GradientPixelType;
  typedef typename Superclass::RealType                      RealType;

  typedef typename Superclass::IndexType                     IndexType;
  typedef typename Superclass::IndexValueType                IndexValueType;
  typedef typename Superclass::SizeType                      SizeType;
  typedef typename Superclass::SpacingType                   SpacingType;
  typedef typename Superclass::DirectionType                 DirectionType;
  typedef typename Superclass::ImageBaseType                 ImageBaseType;

  /** Information from the deformation field class */
  typedef typename Superclass::DeformationFieldType          DeformationFieldType;
  typedef typename Superclass::DeformationFieldPointer       DeformationFieldPointer;
  typedef typename Superclass::DeformationVectorType         DeformationVectorType;
  typedef typename Superclass::TransformType                 TransformType;

  /** Determine the image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int, InputImageType::ImageDimension );

  /** Working image - used for intermediate storage of information */
  // typedef std::pair<InputPixelType, RealType>                IndexWeightPair;
  // typedef itk::Image<IndexWeightPair, ImageDimension>        WorkingImageType;

  /**
   * Number of bins. Important - the intensity values in the fixed and moving images must
   * be integers between 0 and nBins - 1.
   */
  itkSetMacro(Bins, unsigned int)

  /** Get the number of bins */
  itkGetMacro(Bins, unsigned int)

  /**
   * Get the gradient scaling factor. To get the actual gradient of the metric, multiply the
   * gradient output of this filter by the scaling factor. Explanation: for efficiency, the
   * metrics return an arbitrarily scaled vector, such that adding the gradient to the
   * deformation field would INCREASE SIMILARITY. For metrics that are meant to be minimized,
   * this is the opposite of the gradient direction. For metrics that are meant to be maximized,
   * it is the gradient direction.
   */
  virtual double GetGradientScalingFactor() const { return 1.0; }

protected:

  virtual void BeforeThreadedGenerateData();
  // virtual void AfterThreadedGenerateData();
  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId );

protected:
  MultiComponentMutualInfoImageMetric() : m_Bins(32) { }
  ~MultiComponentMutualInfoImageMetric() {}

private:
  MultiComponentMutualInfoImageMetric(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  // Histogram type for MI
  typedef std::vector< std::vector<double *> > HistogramAccumType;

  typename itk::Barrier::Pointer m_Barrier;

  // Number of bins
  unsigned int m_Bins;

  // Combined histogram representation
  struct Histogram
  {
    vnl_matrix<double> Pfm;
    vnl_vector<double> Pf, Pm;
    Histogram(int bins) : Pfm(bins, bins, 0.0), Pf(bins, 0.0), Pm(bins, 0.0) {}
  };

  // Thread data specific to mutual information
  struct MutualInfoThreadData
  {
    HistogramAccumType m_Histogram;
  };

  std::vector<MutualInfoThreadData> m_MIThreadData;

  // Joint probability and marginals for each component
  std::vector<Histogram> m_Histograms;

};



/**
 * A helper filter to remap intensities for mutual information
 */
template <class TInputImage, class TOutputImage>
class MutualInformationPreprocessingFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  typedef MutualInformationPreprocessingFilter<TInputImage, TOutputImage> Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                           Pointer;
  typedef itk::SmartPointer<const Self>                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods) */
  itkTypeMacro( MutualInformationPreprocessingFilter, ImageToImageFilter )

  /** Typedef to describe the output image region type. */
  typedef typename Superclass::OutputImageRegionType         OutputImageRegionType;

  /** Inherit some types from the superclass. */
  typedef typename Superclass::InputImageType                InputImageType;
  typedef typename InputImageType::PixelType                 InputPixelType;
  typedef typename InputImageType::InternalPixelType         InputComponentType;
  typedef typename Superclass::OutputImageType               OutputImageType;
  typedef typename OutputImageType::PixelType                OutputPixelType;
  typedef typename OutputImageType::InternalPixelType        OutputComponentType;

  typedef typename InputImageType::IndexType                 IndexType;
  typedef typename InputImageType::SizeType                  SizeType;


  /** Determine the image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int, InputImageType::ImageDimension );

  /** Set the desired number of bins into which to partition the image */
  itkSetMacro(Bins, unsigned int)

  /**
   * Set the lower quantile (default 0), below which all values are treated as equal
   * to the minimum value.
   */
  itkSetMacro(LowerQuantile, double)

  /**
   * Set the upper quantile (default 0.99), above which all values are treated as equal
   * to the maximum value.
   */
  itkSetMacro(UpperQuantile, double)

protected:
  MutualInformationPreprocessingFilter();
  ~MutualInformationPreprocessingFilter() {}

  virtual void GenerateOutputInformation();

  virtual void BeforeThreadedGenerateData();
  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId );

private:
  MutualInformationPreprocessingFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  // Number of bins
  unsigned int m_Bins;

  // The quantile to map to the upper and lower bins.
  double m_LowerQuantile, m_UpperQuantile;

  // Heap data types
  typedef std::priority_queue<
    InputComponentType, std::vector<InputComponentType>, std::less<InputComponentType> > LowerHeap;

  typedef std::priority_queue<
    InputComponentType, std::vector<InputComponentType>, std::greater<InputComponentType> > UpperHeap;

  void heap_lower_push(LowerHeap &heap, int max_size, const InputComponentType &v)
  {
    if(heap.size() < max_size)
      {
      heap.push(v);
      }
    else if(heap.top() > v)
      {
      heap.pop();
      heap.push(v);
      }
  }

  void heap_upper_push(UpperHeap &heap, int max_size, const InputComponentType &v)
  {
    if(heap.size() < max_size)
      {
      heap.push(v);
      }
    else if(heap.top() < v)
      {
      heap.pop();
      heap.push(v);
      }
  }

  // Per thread data
  struct ThreadData
  {
    // Heaps for minimum and maximum intensities
    LowerHeap heap_lower;
    UpperHeap heap_upper;
  };

  std::vector<ThreadData> m_ThreadData;

  typename itk::Barrier::Pointer m_Barrier;

  std::vector<InputComponentType> m_LowerQuantileValues, m_UpperQuantileValues;
};


#ifndef ITK_MANUAL_INSTANTIATION
#include "MultiComponentMutualInfoImageMetric.txx"
#endif


#endif // MULTICOMPONENTMUTUALINFOIMAGEMETRIC_H