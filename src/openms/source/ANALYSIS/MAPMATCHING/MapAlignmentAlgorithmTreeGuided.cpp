// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Julia Thueringer $
// $Authors: Julia Thueringer $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentAlgorithmTreeGuided.h>
// calculate pearson distance
#include <OpenMS/MATH/STATISTICS/StatisticFunctions.h>
// create binary tree
#include <OpenMS/DATASTRUCTURES/DistanceMatrix.h>
#include <OpenMS/DATASTRUCTURES/BinaryTreeNode.h>
#include <OpenMS/COMPARISON/CLUSTERING/ClusterHierarchical.h>
#include <include/OpenMS/COMPARISON/CLUSTERING/AverageLinkage.h>
// align maps and generate output
#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentTransformer.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/MapAlignmentAlgorithmIdentification.h>

#include <include/OpenMS/APPLICATIONS/MapAlignerBase.h>

using namespace std;

namespace OpenMS
{

  MapAlignmentAlgorithmTreeGuided::MapAlignmentAlgorithmTreeGuided() :
          DefaultParamHandler("MapAlignmentAlgorithmTreeGuided"),
          ProgressLogger()
  {
    defaults_.insert("model:", TOPPMapAlignerBase::getModelDefaults("b_spline"));
    defaults_.setValue("model_type", "b_spline", "Options to control the modeling of retention time transformations from data");
    defaults_.setValidStrings("model_type", ListUtils::create<String>("linear,b_spline,lowess,interpolated"));
    defaults_.insert("align_algorithm:", MapAlignmentAlgorithmIdentification().getDefaults());
    defaults_.setValue("align_algorithm:use_feature_rt", "true");

    defaultsToParam_();
  }

  MapAlignmentAlgorithmTreeGuided::~MapAlignmentAlgorithmTreeGuided()
  {
  }

  void MapAlignmentAlgorithmTreeGuided::updateMembers_()
  {
    align_algorithm_.setParameters(param_.copy("align_algorithm:", true));
    model_param_ = param_.copy("model:",true);
    model_type_ = param_.getValue("model_type");
    model_param_ = model_param_.copy(model_type_+":", true);
  }

  // Similarity functor that provides similarity calculations with the ()-operator for protected type SeqAndRTList
  // that stores retention times given for individual peptide sequences of a feature map
  class MapAlignmentAlgorithmTreeGuided::PeptideIdentificationsPearsonDistance_
  {
  public:
    float operator()(SeqAndRTList& map_first, SeqAndRTList& map_second) const
    {
      // create vectors for both maps containing RTs of identical peptide sequences and
      // get union and intercept amount of peptides
      auto pep1_it = map_first.begin();
      auto pep2_it = map_second.begin();
      vector<double> intercept_rts1;
      vector<double> intercept_rts2;
      float union_size = 0.0;
      while (pep1_it != map_first.end() && pep2_it != map_second.end())
      {
        if (pep1_it->first < pep2_it->first)
        {
          ++pep1_it;
        }
        else if (pep2_it->first < pep1_it->first)
        {
          ++pep2_it;
        }
        else
        {
          double med1 = Math::median(pep1_it->second.begin(), pep1_it->second.end(), true);
          intercept_rts1.push_back(med1);
          double med2 = Math::median(pep2_it->second.begin(), pep2_it->second.end(), true);
          intercept_rts2.push_back(med2);
          ++pep1_it;
          ++pep2_it;
        }
        ++union_size;
      }
      Size intercept_size = intercept_rts1.size();

      // pearsonCorrelationCoefficient(rt_map_i, rt_map_j)
      float pearson_val;
      pearson_val = static_cast<float>(Math::pearsonCorrelationCoefficient(intercept_rts1.begin(), intercept_rts1.end(),
                                                                     intercept_rts2.begin(), intercept_rts2.end()));
      if (pearson_val > 1)
      {
        throw Exception::InvalidRange(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION);
      }

      return pearson_val * intercept_size / union_size;
    }
  }; // end of PeptideIdentificationsPearsonDifference

  // For given peptide identifications extract sequences and store with associated feature RT.
  void MapAlignmentAlgorithmTreeGuided::addPeptideSequences_(const std::vector<PeptideIdentification> &peptides,
          SeqAndRTList &peptide_rts, std::vector<double> &map_range, double feature_rt)
  {
    for (const auto & peptide : peptides)
    {
      if (!peptide.getHits().empty())
      {
        const String& sequence = peptide.getHits()[0].getSequence().toString();
        peptide_rts[sequence].push_back(feature_rt);
        map_range.push_back(feature_rt);
      }
    }
  }

  // For each input map, extract peptide identifications (sequences) of existing features with associated feature RT.
  void MapAlignmentAlgorithmTreeGuided::extractSeqAndRt_(const vector<FeatureMap>& feature_maps,
          vector<SeqAndRTList>& maps_seq_and_rt, vector<vector<double>>& maps_ranges)
  {
    for (Size i = 0; i < feature_maps.size(); ++i)
    {
      for (auto feature_it = feature_maps[i].begin(); feature_maps[i].end() != feature_it; ++feature_it)
      {
        if (!feature_it->getPeptideIdentifications().empty())
        {
          addPeptideSequences_(feature_it->getPeptideIdentifications(), maps_seq_and_rt[i], maps_ranges[i], feature_it->getRT());
        }
      }
      sort(maps_ranges[i].begin(), maps_ranges[i].end());
    }
  }


  // Extract RTs given for individual features of each map, calculate distances for each pair of maps and cluster hierarchical using average linkage.
  void MapAlignmentAlgorithmTreeGuided::buildTree_(std::vector<FeatureMap> &feature_maps, std::vector<BinaryTreeNode> &tree,
          std::vector<std::vector<double>> &maps_ranges)
  {
    vector<SeqAndRTList> maps_seq_and_rt(feature_maps.size());
    extractSeqAndRt_(feature_maps, maps_seq_and_rt, maps_ranges);
    PeptideIdentificationsPearsonDistance_ pep_dist;
    AverageLinkage al;
    DistanceMatrix<float> dist_matrix; // will be filled
    ClusterHierarchical ch;

    ch.cluster<SeqAndRTList, PeptideIdentificationsPearsonDistance_>(maps_seq_and_rt, pep_dist, al, tree, dist_matrix);
  }

  // Align feature maps tree guided using align() of MapAlignmentAlgorithmIdentification and use TreeNode with larger 10/90 percentile range as reference.
  void MapAlignmentAlgorithmTreeGuided::treeGuidedAlignment_(const std::vector<BinaryTreeNode> &tree,
                                                             std::vector<FeatureMap> feature_maps_transformed,
                                                             std::vector<std::vector<double>> &maps_ranges,
                                                             FeatureMap &map_transformed,
                                                             std::vector<Size> &trafo_order)
  {
    Size last_trafo = 0;  // to get final transformation order from map_sets
    vector<TransformationDescription> transformations_align;  // temporary for aligner output

    // helper to memorize rt transformation order
    vector<vector<Size>> map_sets(feature_maps_transformed.size());
    for (Size i = 0; i < feature_maps_transformed.size(); ++i)
    {
      map_sets[i].push_back(i);
    }

    Size ref;
    Size to_transform;

    for (const auto& node : tree)
    {
      // ----------------
      // prepare alignment
      // ----------------
      vector<FeatureMap> to_align;
      //  determine the map with larger RT range for 10/90 percentile (->reference)
      double left_range = maps_ranges[node.left_child][maps_ranges[node.left_child].size()*0.9] - maps_ranges[node.left_child][maps_ranges[node.left_child].size()*0.1];
      double right_range = maps_ranges[node.right_child][maps_ranges[node.right_child].size()*0.9] - maps_ranges[node.right_child][maps_ranges[node.right_child].size()*0.1];

      if (left_range > right_range)
      {
        ref = node.left_child;
        to_transform = node.right_child;
      }
      else
      {
        ref = node.right_child;
        to_transform = node.left_child;
      }
      vector<double> tmp;
      std::merge(maps_ranges[node.right_child].begin(), maps_ranges[node.right_child].end(), maps_ranges[node.left_child].begin(), maps_ranges[node.left_child].end(), std::back_inserter(tmp));

      last_trafo = to_transform;
      to_align.push_back(feature_maps_transformed[to_transform]);
      to_align.push_back(feature_maps_transformed[ref]);

      // ----------------
      // perform alignment
      // ----------------
      align_algorithm_.align(to_align, transformations_align, 1);

      // transform retention times of non-identity for next iteration
      transformations_align[0].fitModel(model_type_, model_param_);
      MapAlignmentTransformer::transformRetentionTimes(feature_maps_transformed[to_transform],
              transformations_align[0], true);

      // combine aligned maps, store in both, because tree always calls smaller number
      // also possible: feature_maps_transformed[smallerNumber] = ..[ref]+..[to_transform]
      // or use pointer
      feature_maps_transformed[ref] += feature_maps_transformed[to_transform];
      feature_maps_transformed[ref].updateRanges();
      feature_maps_transformed[to_transform] = feature_maps_transformed[ref];

      // update order of alignment for both aligned maps
      map_sets[ref].insert(map_sets[ref].end(), map_sets[to_transform].begin(), map_sets[to_transform].end());
      map_sets[to_transform] = map_sets[ref];

      transformations_align.clear();
      to_align.clear();
    }
    // copy last transformed FeatureMap for reference return
    map_transformed = feature_maps_transformed[last_trafo];
    trafo_order = map_sets[last_trafo];
  }

  // Extract original RT ("original_RT" MetaInfo) and transformed RT for each feature to compute RT transformations.
  void MapAlignmentAlgorithmTreeGuided::computeTrafosByOriginalRT_(std::vector<FeatureMap> &feature_maps,
                                                                   FeatureMap &map_transformed,
                                                                   std::vector<TransformationDescription> &transformations,
                                                                   const std::vector<Size> &trafo_order)
  {
    FeatureMap::const_iterator fit = map_transformed.begin();
    TransformationDescription::DataPoints trafo_data_tmp;
    for (auto & map_idx : trafo_order)
    {
      for (Size i = 0; i < feature_maps[map_idx].size(); ++i)
      {
        TransformationDescription::DataPoint point;
        if (fit->metaValueExists("original_RT"))
        {
          point.first = fit->getMetaValue("original_RT");
        }
        else
        {
          point.first = fit->getRT();
        }
        point.second = fit->getRT();
        point.note = fit->getUniqueId();
        trafo_data_tmp.push_back(point);
        ++fit;
      }
      transformations[map_idx] = TransformationDescription(trafo_data_tmp);
      transformations[map_idx].fitModel(model_type_, model_param_);
      trafo_data_tmp.clear();
    }
  }

  void MapAlignmentAlgorithmTreeGuided::computeTransformedFeatureMaps_(vector<FeatureMap>& feature_maps, const vector<TransformationDescription>& transformations)
  {
    for (Size i = 0; i < feature_maps.size(); ++i)
    {
      MapAlignmentTransformer::transformRetentionTimes(feature_maps[i], transformations[i], true);
    }
  }

}