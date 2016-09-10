% Octave/Matlab script for ploting random number generator outputs

% Load random values generated with MT host-based seeds
load('out_lcg_host.tsv')
load('out_xorshift_host.tsv')
load('out_xorshift128_host.tsv')
load('out_mwc64x_host.tsv')

% New figure 
figure
% Plot random values generated with MT host-based seeds
subplot(2,2,1);imagesc(out_lcg_host);title('lcg')
subplot(2,2,2);imagesc(out_mwc64x_host);title('mwc64x')
subplot(2,2,3);imagesc(out_xorshift_host);title('xorshift')
subplot(2,2,4);imagesc(out_xorshift128_host);title('xorshift128')
 
% Optinally use another colormap
% colormap(bone)

% Load random values generated with workitem global IDs
load('out_mwc64x_gid.tsv')
load('out_lcg_gid.tsv')
load('out_xorshift_gid.tsv')
load('out_xorshift128_gid.tsv')

% New figure
figure
% Plot random values generated with workitem global IDs
subplot(2,2,1);imagesc(out_lcg_gid);title('lcg')
subplot(2,2,2);imagesc(out_mwc64x_gid);title('mwc64x')
subplot(2,2,3);imagesc(out_xorshift_gid);title('xorshift')
subplot(2,2,4);imagesc(out_xorshift128_gid);title('xorshift128')

% Optinally use another colormap
% colormap(bone)
