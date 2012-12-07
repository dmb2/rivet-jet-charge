#!/bin/bash -l

JOB_ID=$1
RIVET_PREFIX=/home/dmb60/rivet
RIVET_ANALYSIS_DIR=${RIVET_PREFIX}/Analysis/rivet-jet-charge
OUTDIR=${RIVET_ANALYSIS_DIR}/Pythia6Results
mkdir -p ${OUTDIR}
source ${RIVET_PREFIX}/rivetenv.sh
source ${RIVET_PREFIX}/agileenv.sh
export AGILE_GEN_PATH=${RIVET_PREFIX}/local/generators
cd ${RIVET_ANALYSIS_DIR}
mkfifo fifo_${JOB_ID}.hepmc
agile-runmc Pythia6:426 --beams=LHC:7000 -n 25000 -P MonteCarloParams/Pythia6/wInclusiveProduction.pars -p 'MRPY(1)=100788$1' {-o fifo_${JOB_ID}.hepmc &
rivet --analysis-path=${RIVET_ANALYSIS_DIR} -a MC_GENSTUDY_JETCHARGE --histo-file=${OUTDIR}/Pythia6_part${JOB_ID}.aida