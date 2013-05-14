pro locate_tomo_recon_shareable_library

    ; We use a common block just to store info through calls
    common tomo_recon_common, tomo_recon_shareable_library

    if (n_elements(tomo_recon_shareable_library) eq 0) then begin
        tomo_recon_shareable_library = getenv('TOMO_RECON_SHARE')
        if (tomo_recon_shareable_library eq "") then begin
            file = 'tomoRecon_' + !version.os + '_' + !version.arch
            if (!version.os eq 'Win32') then file=file+'.dll' else file=file+'.so'
            tomo_recon_shareable_library = file_which(file)
        endif
    endif
    if (tomo_recon_shareable_library eq '') then message, 'tomoRecon shareable library not defined'

end
