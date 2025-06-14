
#define CONFIG_DEV_SPI_EN 1
#if CONFIG_DEV_SPI_EN
#include "dev_spi.h"

#include "dev_basic.h"
#include "dev_dma.h"
#include "dev_pin.h"

typedef struct
{
    uint32_t base;
    char device_name[DEVICE_NAME_MAX];
    rcu_periph_enum rcu_clock;
    spi_idx_enum spi_idx;
    uint32_t dma_tx_request;  // DMA request for TX
    uint32_t dma_tx_periph;   // Peripheral address for DMA
} TypeSpiBasicMap;

const TypeSpiBasicMap spi_basic_map[] = {
    {SPI0, "SPI0", RCU_SPI0, IDX_SPI0, DMA_REQUEST_SPI0_TX, (uint32_t)(&SPI_TDATA(SPI0))},
    {SPI1, "SPI1", RCU_SPI1, IDX_SPI1, DMA_REQUEST_SPI1_TX, (uint32_t)(&SPI_TDATA(SPI1))},
    {SPI2, "SPI2", RCU_SPI2, IDX_SPI2, DMA_REQUEST_SPI2_TX, (uint32_t)(&SPI_TDATA(SPI2))},
    {SPI3, "SPI3", RCU_SPI3, IDX_SPI3, DMA_REQUEST_SPI3_TX, (uint32_t)(&SPI_TDATA(SPI3))},
    {SPI4, "SPI4", RCU_SPI4, IDX_SPI4, DMA_REQUEST_SPI4_TX, (uint32_t)(&SPI_TDATA(SPI4))},
    {SPI5, "SPI5", RCU_SPI5, IDX_SPI5, DMA_REQUEST_SPI5_TX, (uint32_t)(&SPI_TDATA(SPI5))},
};

static TypeSpiBasicMap *get_spi_info(uint32_t base) {
    for (int i = 0; i < sizeof(spi_basic_map) / sizeof(TypeSpiBasicMap); i++) {
        if (spi_basic_map[i].base == base) {
            return &spi_basic_map[i];
        }
    }
    return NULL;
}
int DevSpiInit(const DevSpiHandleStruct *ptrDevSpiHandle) {
    TypeSpiBasicMap *spi_info = NULL;
    spi_info                  = get_spi_info(ptrDevSpiHandle->base);
    if (spi_info == NULL) {
        printf("Error: SPI base %x not found in spi_basic_map.\r\n", ptrDevSpiHandle->base);
        while (1);
        return -1;
    }
    DevPinInit(&ptrDevSpiHandle->nss);
    DevPinInit(&ptrDevSpiHandle->clk);
    DevPinInit(&ptrDevSpiHandle->mosi);
    DevPinInit(&ptrDevSpiHandle->miso);
    // Enable the clock for the SPI peripheral
    rcu_periph_clock_enable(spi_info->rcu_clock);
    rcu_spi_clock_config(spi_info->spi_idx, RCU_SPISRC_PLL0Q);
    spi_i2s_deinit(ptrDevSpiHandle->base);
    spi_init(ptrDevSpiHandle->base, &(ptrDevSpiHandle->spi_cfg));
    spi_byte_access_enable(ptrDevSpiHandle->base);
    spi_nss_output_enable(ptrDevSpiHandle->base);
    spi_current_data_num_config(ptrDevSpiHandle->base, 0);
    return 0;
}
int DevSpiDMAWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length) {
    // spi_current_data_num_config(DACSgm3533BspCfg[i].spi_base, 0);  // Clear current data number

    TypeSpiBasicMap *spi_info = NULL;
    spi_info                  = get_spi_info(ptrDevSpiHandle->base);
    if (spi_info == NULL) {
        printf("Error: SPI base %x not found in spi_basic_map.\r\n", ptrDevSpiHandle->base);
        return -1;
    }
    DevDMAHandleStruct *dma_handle = &ptrDevSpiHandle->dam_tx;
    if (dma_handle->request == 0) {
        printf("Error: DMA request not configured for SPI\r\n");
        return -1;
    }
    dma_deinit(dma_handle->base, dma_handle->channel);
    // spi_dma_disable(ptrDevSpiHandle->base, SPI_DMA_TRANSMIT);  // Disable DMA for SPI transmit
    SCB_CleanDCache_by_Addr((uint32_t *)buffer, length);
    dma_handle->dma_cfg.memory0_addr = (uint32_t)buffer;  // Set memory address for DMA
    dma_handle->dma_cfg.number       = length;            // Set number of bytes to transfer

    dma_handle->dma_cfg.periph_addr = spi_info->dma_tx_periph;  // Set peripheral address for DMA

    dma_single_data_mode_init(
        dma_handle->base, dma_handle->channel, &dma_handle->dma_cfg);
    DevPinWrite(&ptrDevSpiHandle->nss, 0);                      // Set NSS low (active state)
    dma_channel_enable(dma_handle->base, dma_handle->channel);  // Enable DMA channel

    spi_enable(ptrDevSpiHandle->base);                                                   // Enable SPI
    spi_dma_enable(ptrDevSpiHandle->base, SPI_DMA_TRANSMIT);                             // Enable DMA for SPI transmit
    spi_master_transfer_start(ptrDevSpiHandle->base, SPI_TRANS_START);                   // Start SPI transfer
    while (dma_flag_get(dma_handle->base, dma_handle->channel, DMA_FLAG_FTF) == RESET);  // Wait for transfer complete
    dma_flag_clear(dma_handle->base, dma_handle->channel, DMA_FLAG_FTF);

    DevPinWrite(&ptrDevSpiHandle->nss, 1);  // Set NSS high (inactive state)
    return 0;
}
// DevSpiWrite
int DevSpiWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length) {
    // spi_current_data_num_config(DACSgm3533BspCfg[i].spi_base, 0);  // Clear current data number

    TypeSpiBasicMap *spi_info = NULL;
    spi_info                  = get_spi_info(ptrDevSpiHandle->base);
    if (spi_info == NULL) {
        printf("Error: SPI base %x not found in spi_basic_map.\r\n", ptrDevSpiHandle->base);
        return -1;
    }
    DevPinWrite(&ptrDevSpiHandle->nss, 0);                      // Set NSS low (active state)


    spi_enable(ptrDevSpiHandle->base);                                                   // Enable SPI
    spi_dma_enable(ptrDevSpiHandle->base, SPI_DMA_TRANSMIT);                             // Enable DMA for SPI transmit
    spi_master_transfer_start(ptrDevSpiHandle->base, SPI_TRANS_START);                   // Start SPI transfer
    for (uint32_t i = 0; i < length; i++) {
        while (spi_i2s_flag_get(ptrDevSpiHandle->base, SPI_STAT_TC) == RESET);  // Wait until transmit buffer is empty
        spi_i2s_data_transmit(ptrDevSpiHandle->base, buffer[i]);                // Transmit data
    }
    while (spi_i2s_flag_get(ptrDevSpiHandle->base, SPI_STAT_TC) == RESET);  // Wait until transmit buffer is empty
    DevPinWrite(&ptrDevSpiHandle->nss, 1);  // Set NSS high (inactive state)
    return 0;
}

uint8_t DevSpiRead(const DevSpiHandleStruct *ptrDevSpiHandle) {
    // return (gpio_input_bit_get(ptrDevSpiHandle->base, ptrDevSpiHandle->pin) == SET) ? 1 : 0;
    return 0;
}

#endif