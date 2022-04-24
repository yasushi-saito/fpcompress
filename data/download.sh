# https://arxiv.org/pdf/2011.02849.pdf

# http://sciviscontest.ieeevis.org/2004/data.html

if false; then
wget -r -nH --no-parent --cut-dirs=2 https://cloud.sdsc.edu/v1/AUTH_sciviscontest/2004/isabeldata

# http://sciviscontest.ieeevis.org/2006/download.html

wget -r -nH --no-parent --cut-dirs=2 https://cloud.sdsc.edu/v1/AUTH_sciviscontest/2006/Data
wget -r -nH --no-parent --cut-dirs=2 https://cloud.sdsc.edu/v1/AUTH_sciviscontest/2006/data_files

# https://klacansky.com/open-scivis-datasets


# https://userweb.cs.txstate.edu/~burtscher/research/datasets/FPsingle/

mkdir -p burtscher_fpsingle
(cd burtscher_fpsingle;
 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/obs_error.sp.spdp;
 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/obs_info.sp.spdp;
 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/obs_spitzer.sp.spdp;
 http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/obs_spitzer.sp.spdp;

 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/num_brain.sp.spdp;
 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/num_comet.sp.spdp;

 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/num_control.sp.spdp;

 wget http://www.cs.txstate.edu/~burtscher/research/datasets/FPsingle/num_plasma.sp.spdp;
)

fi

# https://dps.uibk.ac.at/~fabian/datasets/
# rsim: 2,048 × 11,509 * 1d
# astro_mhd: * 1d
# astro_pt: 512 * 256 * 640 (3d)
# wave: 512 × 512 × 512 (3d)
mkdir -p fabian
(cd fabian
 for url in https://dps.uibk.ac.at/~fabian/datasets/rsim.f32.zst \
            https://dps.uibk.ac.at/~fabian/datasets/rsim.f64.zst \
            https://dps.uibk.ac.at/~fabian/datasets/astro_mhd.f32.zst \
            https://dps.uibk.ac.at/~fabian/datasets/astro_mhd.f64.zst \
            https://dps.uibk.ac.at/~fabian/datasets/astro_pt.f32.zst \
            https://dps.uibk.ac.at/~fabian/datasets/astro_pt.f64.zst \
            https://dps.uibk.ac.at/~fabian/datasets/wave.f32.zst \
            https://dps.uibk.ac.at/~fabian/datasets/wave.f64.zst; do
     wget --no-check-certificate $url
 done
)
